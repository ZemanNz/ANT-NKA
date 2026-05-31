#!/usr/bin/env python3

import random
import serial
import sys
import time
import threading

from lib import frame_send
import rbcx_pb2 as pb

def read_serial(port):
    while True:
        print(port.readline().decode("utf-8"), end='')


if __name__ == "__main__":
    with serial.Serial(sys.argv[1], baudrate=115200) as port:
        threading.Thread(target=read_serial, args=(port, ), daemon=True).start()
        while True:
            position = input()
            if (position == ''):
                msg = pb.CoprocReq(setStupidServo=pb.CoprocReq.SetStupidServo())
                msg.SetStupidServo.disable.SetInParent()
            else:
                msg = pb.CoprocReq(setStupidServo=pb.CoprocReq.SetStupidServo(
                    setPosition=float(position)))
            frame_send(port, msg)

