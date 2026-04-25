#!/bin/sh

mydir=`dirname "$0"`
export LD_LIBRARY_PATH=$mydir/libs:$LD_LIBRARY_PATH

if [ ! -f "$HOME/.jinyong/script/oldtalk.idx" ]; then
    mkdir -p $HOME/.jinyong/script
fi

if [ ! -f "$HOME/.jinyong/data/r3.grp" ]; then
    mkdir -p $HOME/.jinyong/data
    cp libs/r1.grp libs/r2.grp libs/r3.grp $HOME/.jinyong/data/
fi

cd $mydir
./jysdllua
