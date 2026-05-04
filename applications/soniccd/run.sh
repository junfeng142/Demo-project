#!/bin/sh

mydir=`dirname "$0"`

export SDL_VIDEODRIVER=mmiyoo
export LD_LIBRARY_PATH=$mydir/libs:$LD_LIBRARY_PATH

if [ ! -f "/mnt/FunKey/.soniccd/settings.ini" ]; then
    mkdir -p /mnt/FunKey/.soniccd
    cp setting/settings.ini /mnt/FunKey/.soniccd/
fi

cd $mydir
./soniccd
