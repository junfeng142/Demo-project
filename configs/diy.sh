#!/bin/bash
#
# Copyright (c) 2019-2020 P3TERX <https://p3terx.com>
#
# This is free software, licensed under the MIT License.
# See /LICENSE for more information.
#
# https://github.com/P3TERX/Actions-OpenWrt
# File name: diy.sh
# Description: OpenWrt DIY script part 1 (Before Update feeds)
#

# create folder
mkdir -p _opk

# dosbox build
[ -d dosbox ] && patch -p1 < fit_for_dosbox_build.patch

# fceux build
[ -f fceux.png ] && patch -p1 < fit_for_fceux_build.patch

# gambatte build
[ -d gambatte_sdl ] && patch -p1 < fit_for_gambatte_build.patch

# libsdl2 build
[ -d SDL2 ] && patch -p1 < fit_for_sdl2_build.patch

# oswan build
[ -d minizip ] && patch -p1 < fit_for_oswan_build.patch

# crafti build
[ -f worldtask.cpp ] && patch -p1 < fit_for_crafti_build.patch

# ffplay build
[ -d ffplay-gcw0 ] && patch -p1 < fit_for_ffplay_build.patch

# openbor build
[ -d openbor-6391 ] && patch -p1 < fit_for_openbor_build.patch

# cannonball build
if [ -d "cannonboard" ]; then
    sed -i 's/FunKey-sdk/FunKey-sdk-2.3.0/g' Makefile.funkey
    sed -i 's/musleabihf/gnueabihf/g' Makefile.funkey
fi

# gpsp build
if [ -f "game_config.txt" ]; then
    patch -p1 < fit_for_gpsp_build.patch
    cp borders/border1.png _opk/border.png
    cp game_config.txt _opk/
fi

# ngp build
[ -f race-od.man.txt ] && patch -p1 < fit_for_ngp_build.patch

# picodrive build
if [ -f picodrive.map ]; then
    patch -p1 < fit_for_picodrive_build.patch
    mv platform/opendingux/data/skin _opk/
fi

# pocketsnes build
if [ -d "pocketsnes" ]; then
    patch -p1 < fit_for_pocketsnes_build.patch
    cp dist/backdrop.png _opk/
fi

