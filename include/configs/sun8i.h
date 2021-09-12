/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2014 Chen-Yu Tsai <wens@csie.org>
 *
 * Configuration settings for the Allwinner A23 (sun8i) CPU
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * A23 specific configuration
 */

#ifdef CONFIG_USB_EHCI_HCD
#define CONFIG_USB_EHCI_SUNXI
#endif

#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_SECT_SIZE		0x10000
#endif

#ifdef CONFIG_CMD_MTDPARTS
#define MTDIDS_DEFAULT				"nor0=spiflash0"
//#define MTDPARTS_DEFAULT			"mtdparts=spiflash0:512k@0(u-boot)," \
                                    "128k(u-boot-env)," \
                                    "64k(dtb)," \
                                    "3072k(kernel)," \
                                    "-(rootfs)" 
#endif

/*
 * Include common sunxi configuration where most the settings are
 */
#include <configs/sunxi-common.h>

#endif /* __CONFIG_H */
