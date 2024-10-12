#!/bin/sh
USE_CUST_CLOCK=0
mydir=`dirname "$0"`

export HOME=$mydir
export PATH=$mydir:$PATH
export LD_LIBRARY_PATH=$mydir/libs:$LD_LIBRARY_PATH

sv=`cat /proc/sys/vm/swappiness`

cd $mydir

export SDL_VIDEODRIVER=mmiyoo
export EGL_VIDEODRIVER=mmiyoo

if [ -e "/dev/dsp" ]; then
    export SDL_AUDIODRIVER=mmiyoo
else
    export SDL_AUDIODRIVER=alsa
    mv libs/libasound.so.2 libs/__libasound.so.2
fi

if [ "$USE_CUST_CLOCK" == "1" ]; then
    ./cpuclock 1250
fi

# 60 by default
echo 10 > /proc/sys/vm/swappiness

rom=`cat rom.txt`
./drastic --color-depth 16 "$rom"
sync

if [ -f "libs/__libasound.so.2" ]; then
    mv libs/__libasound.so.2 libs/libasound.so.2
fi

echo $sv > /proc/sys/vm/swappiness

if [ "$USE_CUST_CLOCK" == "1" ]; then
    ./cpuclock 1000
fi

