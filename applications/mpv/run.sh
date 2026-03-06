#!/bin/sh

if [ ! -d "$HOME/.mplayer" ]; then #Make directory if it doesn't exist
mkdir $HOME/.mplayer
fi

if [ ! -f "$HOME/.mplayer/setupcomplete" ]; then
rw
cp -f input.conf $HOME/.mplayer
touch $HOME/.mplayer/setupcomplete
ro
fi

LD_LIBRARY_PATH=./ ./mpv -vo sdl -ao sdl -x 320 -y 240 "$1"
