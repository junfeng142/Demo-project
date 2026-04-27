#!/bin/sh

mydir=`dirname "$0"`

export SDL_VIDEODRIVER=mmiyoo
export LD_LIBRARY_PATH=$mydir/libs:$LD_LIBRARY_PATH

cd $mydir
./flare --renderer=sdl
