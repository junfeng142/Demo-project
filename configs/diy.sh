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
mkdir -p ../opk

# cannonball build
if [ -d "cannonball" ]; then
    sed -i 's/FunKey-sdk/FunKey-sdk-2.3.0/g' cannonball/Makefile.funkey
    sed -i 's/musleabihf/gnueabihf/g' cannonball/Makefile.funkey
fi

# dosbox build
[ -d dosbox ] && patch -p1 < ./configs/fit_for_dosbox_build.patch

# fceux build
patch -p1 < ../configs/fit_for_fceux_build.patch

# gambatte build
[ -d gambatte ] && patch -p1 < ./configs/fit_for_gambatte_build.patch

# gpsp build
if [ -d "gpsp" ]; then
    patch -p1 < ./configs/fit_for_gpsp_build.patch
    cp ./gpsp/borders/border1.png ./opk/border.png
    cp ./gpsp/game_config.txt ./opk/game_config.txt
fi

# ngp build
[ -d ngp ] && patch -p1 < ./configs/fit_for_ngp_build.patch

# picodrive build
[ -d picodrive ] && patch -p1 < ./configs/fit_for_picodrive_build.patch

# pocketsnes build
if [ -d "pocketsnes" ]; then
    patch -p1 < ./configs/fit_for_pocketsnes_build.patch
    cp ./pocketsnes/dist/backdrop.png ./opk/backdrop.png
fi

# download sdk
wget https://github.com/junfeng142/Demo-project/releases/download/2025.01.21-2345/FunKey-sdk-2.3.0.tar.gz
tar xvf FunKey-sdk-2.3.0.tar.gz
mv FunKey-sdk-2.3.0 /opt
