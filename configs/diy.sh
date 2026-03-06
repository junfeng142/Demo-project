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

# libretro build
[ -f libretro-super.sh ] && patch -p1 < fit_for_libretro_build.patch

# kernel build
[ -d kernel ] && patch -p1 < fit_for_kernel_build.patch

# uboot build
[ -d include/u-boot ] && patch -p1 < fit_for_uboot_build.patch

