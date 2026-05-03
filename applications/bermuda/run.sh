#!/bin/sh

cd "$(dirname "$0")"

export SDL_VIDEODRIVER=mmiyoo
if [ ! -f "/mnt/FunKey/.bermuda/bermuda.001" ]; then
    mkdir -p /mnt/FunKey/.bermuda
fi

if [ ! -f "/usr/local/lib/timidity/timidity.cfg" ]; then
    mount ./timidity /usr/local/lib/timidity
fi

./bs --widescreen=4:3 --datapath="./DATA"

if [ -f "/usr/local/lib/timidity/timidity.cfg" ]; then
    umount /usr/local/lib/timidity
fi
