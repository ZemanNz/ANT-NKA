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
            msg = pb.CoprocStat(ledsStat=pb.CoprocStat.LedsStat())
            frame_send(port, msg)
            time.sleep(1)

            msg = pb.CoprocStat(buttonsStat=pb.CoprocStat.ButtonsStat(buttonsPressed=0x55))
            frame_send(port, msg)
            time.sleep(1)

