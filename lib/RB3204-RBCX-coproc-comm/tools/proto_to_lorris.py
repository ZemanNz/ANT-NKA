#!/usr/bin/env python3

from typing import IO, List, Any, Generator

import argparse
import binascii
import os
import json
import struct
import subprocess
import sys
import tempfile
import threading
import socket

from google.protobuf import reflection, descriptor, message_factory
from google.protobuf.message import Message
from google.protobuf.descriptor import FieldDescriptor
from google.protobuf.descriptor_pb2 import FileDescriptorSet

from lib import frame_receive

DESCRIPTION="""This is a mezikus between protobuffer RBCX communication and Lorris.

Usage to interact with STM32 co-processor in place of the ESP32:

    %s ./rbcx.proto StmMessage /dev/ttyUSB0

 * Copy the JSON this script generates at start into clipboard.
 * Connect to localhost:8181 in Lorris version >= 940
 * Open lorris_preset.cldta
 * Open the filters menu on top right in the analyzer
 * Click the JSON mode button on top right
 * Paste in the JSON from this script.

""" % sys.argv[0]

INDEX_LEN = 1
INDEX_PATH = 2

class ConnectionReader(threading.Thread):
    def __init__(self, conn, input):
        super().__init__(daemon=True)
        self.conn = conn
        self.input = input
    
    def run(self):
        while True:
            try:
                data = self.conn.recv(512)
                self.input.write(data)
                try:
                    self.input.flush()
                except Exception:
                    pass
            except socket.timeout:
                continue
            except (ConnectionError, OSError) as e:
                print(e)
                return

class ServerThread(threading.Thread):
    def __init__(self, port, input):
        super().__init__(daemon=True)

        self.port = port
        self.input = input
        self.connections = []
        self.mu = threading.Lock()

    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        server_address = ('localhost', self.port)
        print('starting up on {} port {}'.format(*server_address))
       
        sock.bind(server_address)

        sock.listen(16)

        while True:
            connection, client_address = sock.accept()
            connection.settimeout(0.2)

            ConnectionReader(connection, self.input).start()

            print('connection from', client_address)
            with self.mu:
                self.connections.append(connection)

    def send(self, data: bytes):
        with self.mu:
            for c in list(self.connections):
                try:
                    c.sendall(data)
                except ConnectionError as e:
                    c.close()
                    self.connections.remove(c)
                    print(e)
                except Exception as e:
                    print(e)

def load_descriptor_set(protoc_bin, proto_path):
    with tempfile.TemporaryDirectory("lorris_temp_proto") as tmpdir:
        descr_path = os.path.join(tmpdir, "proto.descr")
        subprocess.check_call([ protoc_bin, "--descriptor_set_out=" + descr_path, proto_path ])

        with open(descr_path, "rb") as f:
            s = FileDescriptorSet()
            s.ParseFromString(f.read())
            return s

def parse_message(desc, filters, parents=[], indent=0):
    msg_name = [ p.name for p in parents ]
    msg_path = [ p.number for p in parents ]
    for fd in desc.fields:
        #print("  " * indent, fd.number, fd.name, fd.label, fd.type, fd.message_type)
        if fd.type == FieldDescriptor.TYPE_MESSAGE:
            parse_message(fd.message_type, filters, parents + [ fd ], indent + 1)
        elif fd.type == FieldDescriptor.TYPE_GROUP:
            raise NotImplementedError()
        else:
            path = msg_path + [ fd.number ]
            name = ".".join(msg_name + [ fd.name ])
            dividers = [ INDEX_PATH + len(path) ]
            if fd.label == FieldDescriptor.LABEL_REPEATED:
                name += "[]"
                dividers.append(dividers[0] + 1)

            filters.append({
                "name": name,
                "dividers": dividers,
                "conditions": [ {
                    "type": 2, # byte
                    "idx": INDEX_PATH + i,
                    "value": num, 
                } for i, num in enumerate(path) ],
            })

def encode_scalar(path: List[int], fd: FieldDescriptor, val: Any) -> bytes:
    from google.protobuf.descriptor import FieldDescriptor as F

    variable_len = False
    if fd.type in (F.TYPE_INT32, F.TYPE_FIXED32, F.TYPE_UINT32, F.TYPE_ENUM, F.TYPE_SFIXED32, F.TYPE_SINT32):
        val_fmt = "i"
    elif fd.type in (F.TYPE_INT64, F.TYPE_FIXED64, F.TYPE_UINT64, F.TYPE_SFIXED64, F.TYPE_SINT64):
        val_fmt = "q"
    elif fd.type == F.TYPE_FLOAT:
        val_fmt = "f"
    elif fd.type  == F.TYPE_DOUBLE:
        val_fmt = "d"
    elif fd.type  == F.TYPE_BOOL:
        val_fmt = "?"
    elif fd.type  in (F.TYPE_STRING, F.TYPE_BYTES):
        val_fmt = "s"
        variable_len = True
    else:
        raise NotImplementedError("Not implemented type: %d" % typ)

    if not variable_len:
        val_len = struct.calcsize(val_fmt)
    else:
        val_len = len(val)

    packet_data = [ 0xFF, len(path) + val_len ] + path + [ val ]
    fmt = "<BB" + ("B" * len(path)) + val_fmt
    #print(fmt, packet_data)
    return struct.pack(fmt, *packet_data)

def encode_message(msg: Message, parents: List[FieldDescriptor] = []) -> Generator[bytes, None, None]:
    msg_path = [ p.number for p in parents ]
    for fd, val in msg.ListFields():
        if fd.type == FieldDescriptor.TYPE_MESSAGE:
            for d in encode_message(val, parents + [ fd ]):
                yield d
        elif fd.type == FieldDescriptor.TYPE_GROUP:
            raise NotImplementedError()
        else:
            path = msg_path + [ fd.number ]
            if fd.label == FieldDescriptor.LABEL_REPEATED:
                for i, v in enumerate(val):
                    yield encode_scalar(path + [ i ], fd, v)
            else:
                yield encode_scalar(path, fd, val)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=DESCRIPTION, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("proto", help="path to the .proto file")
    parser.add_argument("message", help="Name of the incomming message")
    parser.add_argument("input", help="Path to the serial port or file")
    parser.add_argument("-p", "--port", type=int, help="local proxy port", default=8181)
    parser.add_argument("-b", "--baudrate", type=int, default=115200, help="serial port baudrate")
    parser.add_argument("-r", "--raw", action="store_true", help="Open the port as a file instead of as serial port.")
    parser.add_argument("--protoc", default="protoc", help="Path to the protoc binary")
    params = parser.parse_args()

    descriptor_set = load_descriptor_set(params.protoc, params.proto)

    messages = message_factory.GetMessages(descriptor_set.file)
    msg_type = messages[params.message]

    filters = []
    parse_message(msg_type().DESCRIPTOR, filters)

    print(json.dumps(filters, indent=4))

    input = None
    is_stdin = params.input.strip() == "-"
    if is_stdin:
        input = sys.stdin.buffer
    elif params.raw:
        input = open(params.input, "rb")
    else:
        input = serial.Serial(params.input, baudrate=params.baudrate)

    server = ServerThread(params.port, input if not is_stdin else sys.stderr.buffer)
    server.start()

    try:
        while True:
            msg = frame_receive(sys.stdin.buffer, msg_type)
            print("%s {\n  %s\n}" % (params.message, str(msg).replace("\n", "\n  ").rstrip()))
            for data in encode_message(msg):
                server.send(data)
    finally:
        input.close()
