#!/bin/sh
dd if=/dev/zero of=bin/orangepizero_lede.bin bs=1M count=16
dd if=uboot_spl/u-boot-sunxi-with-spl.bin of=bin/orangepizero_lede.bin bs=1K conv=notrunc
dd if=lede/sun8i-h2-plus-orangepi-zero.dtb of=bin/orangepizero_lede.bin bs=1K seek=640 conv=notrunc
dd if=lede/zImage of=bin/orangepizero_lede.bin bs=1K seek=704 conv=notrunc
#dd if=lede/root.squashfs of=bin/orangepizero_lede.bin bs=1K seek=3776 conv=notrunc
dd if=lede/rootfs.bin of=bin/orangepizero_lede.bin bs=1K seek=3776 conv=notrunc
