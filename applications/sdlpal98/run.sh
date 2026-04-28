#!/bin/sh

mydir=`dirname "$0"`

if [ ! -d "$HOME/.sdlpal98" ]; then
    mkdir -p $HOME/.sdlpal98
fi

export SDL_VIDEODRIVER=mmiyoo

cd $mydir

./sdlpal98
