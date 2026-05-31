from typing import IO, Type
from google.protobuf.message import Message
from io import BytesIO
import struct

from . import cobs

def frame_encode(msg: Message) -> bytes:
    pb_data = msg.SerializeToString()
    return cobs.encode(pb_data)

def frame_send(file: IO[bytes], msg: Message):
    data = frame_encode(msg)
    file.write(struct.pack("<BB", 0, len(data)))
    file.write(data)

def frame_receive(file: IO[bytes], msg_type: Type[Message]):
    state = 0
    frame_len = 0
    buf = BytesIO()

    while True:
        b = file.read(1)[0]

        if b == 0x00:
            state = 1
        elif state == 1:
            frame_len = b
            buf = BytesIO()
            state = 2
        elif state == 2:
            buf.write(bytes([ b ]))
            if len(buf.getbuffer()) == frame_len:
                break

    pb_data = cobs.decode(buf.getbuffer())
    msg = msg_type()
    msg.ParseFromString(pb_data)
    return msg
