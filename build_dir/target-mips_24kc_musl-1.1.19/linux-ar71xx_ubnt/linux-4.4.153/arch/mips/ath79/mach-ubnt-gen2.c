/*
 *  Ubiquiti UniFi Gen2 board support
 *
 *  Copyright (C) 2015-2016 P. Wassi <p.wassi at gmx.at>
 *
 *  Derived from: mach-ubnt-unifiac.c
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/ath9k_platform.h>
#include <linux/etherdevice.h>

#include <asm/mach-ath79/ath79.h>
#include <asm/mach-ath79/irq.h>
#include <asm/mach-ath79/ar71xx_regs.h>

#include <linux/platform_data/phy-at803x.h>
#include <linux/ar8216_platform.h>

#ifdef CONFIG_ATH79_MACH_UBNT_UNIFILTE
#include <linux/spi/spi.h>
#include "dev-spi.h"
#endif

#include "common.h"
#include "dev-ap9x-pci.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-wmac.h"
#include "dev-usb.h"
#include "machtypes.h"

#include <ubnthal_system.h>
#include <ubntboard.h>

#define UNIFIAC_KEYS_POLL_INTERVAL	20
#define UNIFIAC_KEYS_DEBOUNCE_INTERVAL	(3 * UNIFIAC_KEYS_POLL_INTERVAL)

#define UNIFIAC_GPIO_BTN_RESET		2

#define UBNT_EEPROM                     0x1fff0000
#define UNIFIAC_MAC0_OFFSET             0x0000
#define UNIFI_GEN2_BOARD_ID_OFFSET      0x000c
#define UNIFIAC_WMAC_CALDATA_OFFSET     0x1000
#define UNIFIAC_PCI_CALDATA_OFFSET      0x5000


static struct flash_platform_data ubnt_unifiac_flash_data = {
	/* mx25l12805d and mx25l12835f have the same JEDEC ID */
	.type = "mx25l12805d",
	.name = "ath-nor0"
};

#ifdef CONFIG_ATH79_MACH_UBNT_UNIFILTE
#define INSTANT_LTE_SSD1317_CS 11
static struct ath79_spi_controller_data instant_lte_spi0_data = {
	.cs_type = ATH79_SPI_CS_TYPE_INTERNAL,
	.is_flash = true,
	.cs_line = 0,
};

static struct ath79_spi_controller_data instant_lte_spi1_data = {
	.cs_type = ATH79_SPI_CS_TYPE_GPIO,
	.cs_line = INSTANT_LTE_SSD1317_CS,
	//.cs_type = ATH79_SPI_CS_TYPE_INTERNAL,
	//.cs_line = 1,
	.is_flash = false,
};
 
static struct spi_board_info instant_lte_spi_info[] = {
		{
		.bus_num	= 0,
		.chip_select	= 0,
		.max_speed_hz	= 25000000,
		.modalias	= "m25p80",
		.platform_data	= NULL,
		.controller_data = &instant_lte_spi0_data,
	},
	{
		.bus_num	= 0,
		.chip_select	= 1,
		.max_speed_hz	= 25000000,
		.modalias	= "solomon,ssd1317fb-spi",
		.platform_data	= NULL,
		.controller_data = &instant_lte_spi1_data,
	}
};

static struct ath79_spi_platform_data instant_lte_spi_data = {
	.bus_num		= 0,
	.num_chipselect		= 2,
};

#else	// ifdef CONFIG_ATH79_MACH_UBNT_UNIFILTE
#define UNIFIAC_GPIO_LED_WHITE		7
#define UNIFIAC_GPIO_LED_BLUE		8

static struct gpio_led ubnt_unifiac_leds_gpio[] __initdata = {
	{
		.name		= "ubnt:white:dome",
		.gpio		= UNIFIAC_GPIO_LED_WHITE,
		.active_low	= 0,
		.default_trigger = "external0",
	}, {
		.name		= "ubnt:blue:dome",
		.gpio		= UNIFIAC_GPIO_LED_BLUE,
		.active_low	= 0,
		.default_trigger = "external1",
	}
};
#endif	// ifdef CONFIG_ATH79_MACH_UBNT_UNIFILTE


static struct gpio_keys_button ubnt_unifiac_gpio_keys[] __initdata = {
	{
		.desc			= "reset",
		.type			= EV_KEY,
		.code			= KEY_RESTART,
		.debounce_interval	= UNIFIAC_KEYS_DEBOUNCE_INTERVAL,
		.gpio			= UNIFIAC_GPIO_BTN_RESET,
		.active_low		= 1,
	}
};

#define UBNT_GEN2_GPIO_MDC			3
#define UBNT_GEN2_GPIO_MDIO			4

struct unifi_ap_model {
	u16 id;
	const char *name;
	int is_switch;
	int is_usb;
};

static struct unifi_ap_model unifi_models[] __initdata = {
	{ SYSTEMID_UAPACLITE, NAME_UAPACLITE_DRAGONFLY, 0, 0},
	{ SYSTEMID_UAPACLR, NAME_UAPACLR_DRAGONFLY, 0, 0},
	{ SYSTEMID_UAPACMESH, NAME_UAPACMESH_DRAGONFLY, 0, 0},
	{ SYSTEMID_UAPACPRO, NAME_UAPACPRO_DRAGONFLY, 1, 1},
	{ SYSTEMID_ULTEUS, NAME_ULTEUS, 1, 1},
	{ SYSTEMID_ULTEPROEU, NAME_ULTEUS, 1, 1},
	{ SYSTEMID_ULTEPROUS, NAME_ULTEUS, 1, 1},
	{ SYSTEMID_ULTEFLEXEU, NAME_ULTEUS, 1, 1},
	{ SYSTEMID_ULTEFLEXUS, NAME_ULTEUS, 1, 1},
	{ SYSTEMID_ULTEPROAU, NAME_ULTEUS, 1, 1},
	{ SYSTEMID_ULTEFLEXAU, NAME_ULTEUS, 1, 1},
	{ SYSTEMID_UAPACEDU, NAME_UAPACEDU_DRAGONFLY, 1, 1},
	{ SYSTEMID_UAPACMSHPRO, NAME_UAPACMSHPRO_DRAGONFLY, 1, 0},
	{ SYSTEMID_UAPACINWALL,	NAME_UAPACINWALL_DRAGONFLY, 1, 0},
	{ SYSTEMID_UAPACIWPRO, NAME_UAPACINWALLPRO_DRAGONFLY, 1, 0},
	{ 0, NULL, 0, 0}
};


static struct ar8327_pad_cfg ubnt_gen2_ar8337_pad0_cfg = {
	.mode = AR8327_PAD_MAC_SGMII,
	.sgmii_delay_en = true,
};

static struct ar8327_platform_data ubnt_gen2_ar8337_data = {
	.pad0_cfg = &ubnt_gen2_ar8337_pad0_cfg,
	.port0_cfg = {
		.force_link = 1,
		.speed = AR8327_PORT_SPEED_1000,
		.duplex = 1,
		.txpause = 1,
		.rxpause = 1,
	},
};

static struct at803x_platform_data ubnt_gen2_ar8033_data = {
	.disable_smarteee = 1,
	.enable_rgmii_rx_delay = 1,
	.enable_rgmii_tx_delay = 1,
	.fixup_rgmii_tx_delay = 1,
};

static struct mdio_board_info ubnt_gen2_mdio0_info[] = {
	{
		.bus_id = "ag71xx-mdio.0",
	},
};

static void __init ubnt_gen2_setup(void)
{
	u8 *eeprom = (u8 *) KSEG1ADDR(UBNT_EEPROM);
	u16 *board_id = (u16 *) KSEG1ADDR(UBNT_EEPROM + UNIFI_GEN2_BOARD_ID_OFFSET);
	struct unifi_ap_model *dev = NULL;
	int i = 0;

	for (i = 0; unifi_models[i].id != 0; i++) {
		if (unifi_models[i].id == *board_id) {
			dev = &unifi_models[i];
			break;
		}
	}
	
	printk ("Board: Ubiquiti %s", (dev ? dev->name : "??"));

#ifdef CONFIG_ATH79_MACH_UBNT_UNIFILTE
	// Disable JTAG function for GPIO 15 control (PoE out D flip-flop clock)
	ath79_gpio_function_enable(BIT(1));

	// gpio 7 is reset for LTE.
	// gpio 11 is SPI CS.
	// gpio 20 and 21 are D/C# and RES#.
	ath79_gpio_direction_select(11,1);
	ath79_gpio_direction_select(20,0);
	ath79_gpio_direction_select(21,0);
	ath79_gpio_direction_select(7,1);
	instant_lte_spi_info[0].platform_data = &ubnt_unifiac_flash_data;
	ath79_register_spi(&instant_lte_spi_data, instant_lte_spi_info, ARRAY_SIZE(instant_lte_spi_info));
#else
	ath79_register_m25p80(&ubnt_unifiac_flash_data);
#endif

	if (dev && dev->is_usb)
		ath79_register_usb();
	ath79_init_mac(ath79_eth0_data.mac_addr,
	               eeprom + UNIFIAC_MAC0_OFFSET, 0);

	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_SGMII;
	ath79_eth0_data.speed = SPEED_1000;
	ath79_eth0_data.duplex = DUPLEX_FULL;

	ath79_gpio_output_select(UBNT_GEN2_GPIO_MDC, QCA956X_GPIO_OUT_MUX_GE0_MDC);
	ath79_gpio_output_select(UBNT_GEN2_GPIO_MDIO, QCA956X_GPIO_OUT_MUX_GE0_MDO);

	ath79_register_mdio(0, 0);

	if (dev) {
		struct mdio_board_info *mdio_info = &ubnt_gen2_mdio0_info[0];
		if (dev->is_switch) {	// AR8334 switch
			ath79_eth0_data.phy_mask = BIT(0);
			mdio_info->phy_addr = 0;
			mdio_info->platform_data = &ubnt_gen2_ar8337_data;
		} else {			// AR8033 PHY
			ath79_eth0_data.phy_mask = BIT(4);
			mdio_info->phy_addr = 4;
			mdio_info->platform_data = &ubnt_gen2_ar8033_data;
		}
	}
	mdiobus_register_board_info(ubnt_gen2_mdio0_info,
			ARRAY_SIZE(ubnt_gen2_mdio0_info));
	ath79_eth0_pll_data.pll_10 = 0x00001313;

	ath79_eth0_data.mii_bus_dev = &ath79_mdio0_device.dev;
	ath79_register_eth(0);


	ath79_register_wmac(eeprom + UNIFIAC_WMAC_CALDATA_OFFSET, NULL);


	ap91_pci_init(eeprom + UNIFIAC_PCI_CALDATA_OFFSET, NULL);

#ifndef CONFIG_ATH79_MACH_UBNT_UNIFILTE
	ath79_register_leds_gpio(-1, ARRAY_SIZE(ubnt_unifiac_leds_gpio),
	                         ubnt_unifiac_leds_gpio);
#endif

	ath79_register_gpio_keys_polled(-1, UNIFIAC_KEYS_POLL_INTERVAL,
	                                ARRAY_SIZE(ubnt_unifiac_gpio_keys),
	                                ubnt_unifiac_gpio_keys);
}

MIPS_MACHINE(ATH79_MACH_UBNT_GEN2, "UBNTGen2", "Ubiquiti Networks Inc. (c) gen2", ubnt_gen2_setup);
