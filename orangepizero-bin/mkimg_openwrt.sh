#!/bin/sh
dd if=/dev/zero of=bin/orangepizero_openwrt.bin bs=1M count=16
dd if=uboot_spl/u-boot-sunxi-with-spl.bin of=bin/orangepizero_openwrt.bin bs=1K conv=notrunc
dd if=openwrt/sun8i-h2-plus-orangepi-zero.dtb of=bin/orangepizero_openwrt.bin bs=1K seek=640 conv=notrunc
dd if=openwrt/zImage of=bin/orangepizero_openwrt.bin bs=1K seek=704 conv=notrunc
#dd if=openwrt/root.squashfs of=bin/orangepizero_openwrt.bin bs=1K seek=3776 conv=notrunc
dd if=openwrt/rootfs.bin of=bin/orangepizero_openwrt.bin bs=1K seek=3776 conv=notrunc
