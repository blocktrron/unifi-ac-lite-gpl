/*
 *  Atheros SoC built-in WMAC device support
 *
 *  Copyright (C) 2017 Ubiquiti Networks, Inc.
 *
 *  Parts of this file are based on Atheros 2.6.15/2.6.31 BSP
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/ubntdev_platform.h>

#include <asm/mach-ath79/ath79.h>
#include <asm/mach-ath79/ar71xx_regs.h>
#include "dev-ubntdev.h"

struct ubntdev_platform_data ath79_ubntdev_data;


static struct platform_device ath79_ubntdev_device = {
	.name		= "ubnt_virtual_dev",
	.id		= -1,
	.resource	= NULL,
	.num_resources	= 0,
	.dev = {
		.platform_data = &ath79_ubntdev_data,
	},
};

void ath79_init_ubntdev_pdata(u8 *art_data)
{
	ath79_ubntdev_device.name = "ubntdev";

	if (art_data)
		memcpy(ath79_ubntdev_data.eeprom_data, art_data,
		       sizeof(ath79_ubntdev_data.eeprom_data));
}

void __init ath79_register_ubntdev(u8 *art_data)
{
	ath79_init_ubntdev_pdata(art_data);
	platform_device_register(&ath79_ubntdev_device);
}
