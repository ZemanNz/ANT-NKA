#!/bin/sh

# Install protobuf-compiler and pip3 install nanopb

protoc -orbcx.pb rbcx.proto
nanopb_generator -L '#include "nanopb/%s"' -D src rbcx.pb
protoc --python_out=tools rbcx.proto nanopb.proto
