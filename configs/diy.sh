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
mkdir -p opk

# download sdk
cd && wget https://github.com/junfeng142/Demo-project/releases/download/2025.01.21-1309/FunKey-sdk-2.3.0.tar.gz
cd && tar xvf FunKey-sdk-2.3.0.tar.gz
cd && mv FunKey-sdk-2.3.0 /opt

# cannonball build
[ -d cannonball ] && patch -p1 < ./configs/fit_for_cannonball_build.patch

# dosbox build
[ -d dosbox ] && patch -p1 < ./configs/fit_for_dosbox_build.patch

# fecux build
[ -d fecux ] && patch -p1 < ./configs/fit_for_fecux_build.patch

# gambatte build
[ -d gambatte ] && patch -p1 < ./configs/fit_for_gambatte_build.patch

# gpsp build
if [ -d "gpsp" ]; then
    patch -p1 < ./configs/fit_for_gpsp_build.patch
    cp ./gpsp/borders/border1.png ./opk/border.png
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

