#!/bin/bash

export EXTRA_CFLAGS="-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64"
export CC=/opt/FunKey-sdk-2.3.0/bin/arm-funkey-linux-gnueabihf-gcc

./configure --target-device miyoo --disable notify-frontend --disable web-frontend --disable log-frontend
make clean
make
