#!/bin/sh

cd "$(dirname "$0")"

export SDL_VIDEODRIVER=mmiyoo

if [ -f "DIABDAT.mpq" ] || [ -f "spawn.mpq" ]; then
  ./devilutionx
else
	echo "Missing DIABDAT.mpq!"
fi
