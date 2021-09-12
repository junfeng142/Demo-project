#!/bin/sh
dd if=/dev/zero of=bin/firmware_openwrt.bin bs=1K count=15744
dd if=openwrt/sun8i-h2-plus-orangepi-zero.dtb of=bin/firmware_openwrt.bin bs=1K conv=notrunc
dd if=openwrt/zImage of=bin/firmware_openwrt.bin bs=1K seek=64 conv=notrunc
dd if=openwrt/root.squashfs of=bin/firmware_openwrt.bin bs=1K seek=3136 conv=notrunc
#dd if=openwrt/rootfs.bin of=bin/firmware_openwrt.bin bs=1K seek=3136 conv=notrunc

