#!/bin/sh
export STAGING_DIR=$PWD/tmp
export PATH=/home/javonca/openwrt/staging_dir/toolchain-arm_cortex-a7+neon-vfpv4_gcc-7.5.0_musl_eabi/bin:$PATH
export CROSS_COMPILE=arm-openwrt-linux-
export ARCH=arm 

make
