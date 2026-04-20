#!/bin/sh

CONFIGDIR=$HOME/.config/gmu
CONFIGFILE=$CONFIGDIR/gmu.conf
if [ ! -d "$CONFIGFILE" ]; then
  mkdir -p $CONFIGDIR
fi

if [ ! -f "$CONFIGFILE" ]; then
  cp gmu.miyoo.conf $CONFIGFILE
fi

cd `dirname $0`
SDL_NOMOUSE=1 LD_LIBRARY_PATH=libs/ ./gmu.bin -c $CONFIGFILE
