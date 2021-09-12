#!/bin/sh
dd if=/dev/zero of=bin/firmware_lede.bin bs=1K count=15744
dd if=lede/sun8i-h2-plus-orangepi-zero.dtb of=bin/firmware_lede.bin bs=1K conv=notrunc
dd if=lede/zImage of=bin/firmware_lede.bin bs=1K seek=64 conv=notrunc
dd if=lede/root.squashfs of=bin/firmware_lede.bin bs=1K seek=3136 conv=notrunc
#dd if=lede/rootfs.bin of=bin/firmware_lede.bin bs=1K seek=3136 conv=notrunc

#dd if=1.bin bs=1M count=2108 of=armbian.bin

