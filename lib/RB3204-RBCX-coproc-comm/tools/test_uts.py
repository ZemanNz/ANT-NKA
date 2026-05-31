#!/usr/bin/env python3

import random
import serial
import sys
import time
import threading
from google.protobuf import text_format

from lib import frame_send, frame_receive
import rbcx_pb2 as pb

def read_serial(port):
    while True:
        print(text_format.MessageToString(frame_receive(port, pb.CoprocStat), as_one_line=True))


if __name__ == "__main__":
    with serial.Serial(sys.argv[1], baudrate=921600) as port:
        threading.Thread(target=read_serial, args=(port, ), daemon=True).start()
        while True:
            ultrasoundReq = pb.CoprocReq.UltrasoundReq(utsIndex=3)
            ultrasoundReq.singlePing.SetInParent()
            msg = pb.CoprocReq(ultrasoundReq=ultrasoundReq)
            frame_send(port, msg)
            time.sleep(0.5)
