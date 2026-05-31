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
        msg = frame_receive(port, pb.CoprocStat);
        if not msg.HasField('powerAdcStat'):
            print(text_format.MessageToString(msg, as_one_line=True))


if __name__ == "__main__":
    with serial.Serial(sys.argv[1], baudrate=921600) as port:
        threading.Thread(target=read_serial, args=(port, ), daemon=True).start()
        motorIndex = int(sys.argv[2]) if len(sys.argv) > 2 else 1
        msg = pb.CoprocReq(motorReq=pb.CoprocReq.MotorReq(motorIndex=motorIndex))
        msg.motorReq.getState.SetInParent();
        frame_send(port, msg)
        p = 1000
        i = 0
        d = 5000
        #a = int(input("AccMax:"))
        msg = pb.CoprocReq(motorReq=pb.CoprocReq.MotorReq(motorIndex=motorIndex, setPositionRegCoefs=pb.RegCoefs(p=p, i=i, d=d)))
        frame_send(port, msg)
        #msg = pb.CoprocReq(motorReq=pb.CoprocReq.MotorReq(motorIndex=motorIndex, setConfig=pb.MotorConfig(maxAccel=1000)))
        #frame_send(port, msg)
        while True:
            #position = 
            msg = pb.CoprocReq(motorReq=pb.CoprocReq.MotorReq(
                motorIndex=motorIndex,
                #setPower=int(input("P:"))))
                #setVelocity=int(input("v:"))))
                addPosition=pb.CoprocReq.MotorReq.SetPosition(targetPosition=int(input("s:")), runningVelocity=2500)))
            frame_send(port, msg)
