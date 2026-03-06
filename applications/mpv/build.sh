#!/bin/bash

export CC=/opt/FunKey-sdk-2.3.0/bin/arm-funkey-linux-gnueabihf-gcc

cd MPlayer-1.4
./configure --target=arm-funkey-linux
make -j4 V=s
