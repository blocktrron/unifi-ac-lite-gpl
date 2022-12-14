/*
 * drivers/net/phy/at803x.c
 *
 * Driver for Atheros 803x PHY
 *
 * Author: Matus Ujhelyi <ujhelyi.m@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/phy.h>
#include <linux/mdio.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_data/phy-at803x.h>
#include <linux/rtnetlink.h>

#define AT803X_INTR_ENABLE			0x12
#define AT803X_INTR_STATUS			0x13
#define AT803X_SMART_SPEED			0x14
#define AT803X_LED_CONTROL			0x18
#define AT803X_WOL_ENABLE			0x01
#define AT803X_DEVICE_ADDR			0x03
#define AT803X_LOC_MAC_ADDR_0_15_OFFSET		0x804C
#define AT803X_LOC_MAC_ADDR_16_31_OFFSET	0x804B
#define AT803X_LOC_MAC_ADDR_32_47_OFFSET	0x804A
#define AT803X_MMD_ACCESS_CONTROL		0x0D
#define AT803X_MMD_ACCESS_CONTROL_DATA		0x0E
#define AT803X_FUNC_DATA			0x4003
#define AT803X_INER				0x0012
#define AT803X_INER_INIT			0xec00
#define AT803X_INSR				0x0013
#define AT803X_REG_CHIP_CONFIG			0x1f
//: CHIP CONFIG field
#define AT803X_BT_BX_REG_SEL			0x8000
#define AT803X_PRIORITY_SEL			0x0400
#define AT803X_MODE_CFG_MASK			0xFFF0
#define AT803X_MODE_CFG_BASET_RGMII		0x0000
#define AT803X_MODE_CFG_BASET_SGMII		0x0001
#define AT803X_MODE_CFG_RC_AUTO_MODE		0x000B

#define AT803X_SGMII_ANEG_EN			0x1000

#define AT803X_PCS_SMART_EEE_CTRL3			0x805D
#define AT803X_SMART_EEE_CTRL3_LPI_TX_DELAY_SEL_MASK	0x3
#define AT803X_SMART_EEE_CTRL3_LPI_TX_DELAY_SEL_SHIFT	12
#define AT803X_SMART_EEE_CTRL3_LPI_EN			BIT(8)

#define AT803X_DEBUG_ADDR			0x1D
#define AT803X_DEBUG_DATA			0x1E
#define AT803X_DBG0_REG				0x00
#define AT803X_DEBUG_RGMII_RX_CLK_DLY		BIT(8)
#define AT803X_DEBUG_SYSTEM_MODE_CTRL		0x05
#define AT803X_DEBUG_RGMII_TX_CLK_DLY		BIT(8)

#define AT803X_PHY_ID_MASK			0xffffffef
#define ATH8030_PHY_ID				0x004dd076
#define ATH8031_PHY_ID				0x004dd074
#define ATH8032_PHY_ID				0x004dd023
#define ATH8035_PHY_ID				0x004dd072

MODULE_DESCRIPTION("Atheros 803x PHY driver");
MODULE_AUTHOR("Matus Ujhelyi");
MODULE_LICENSE("GPL");

struct at803x_priv {
	bool phy_reset:1;
	struct gpio_desc *gpiod_reset;
	int prev_speed;
};

struct at803x_context {
	u16 bmcr;
	u16 advertise;
	u16 control1000;
	u16 int_enable;
	u16 smart_speed;
	u16 led_control;
};

static u16
at803x_dbg_reg_rmw(struct phy_device *phydev, u16 reg, u16 clear, u16 set)
{
	struct mii_bus *bus = phydev->bus;
	int val;

	mutex_lock(&bus->mdio_lock);

	bus->write(bus, phydev->addr, AT803X_DEBUG_ADDR, reg);
	val = bus->read(bus, phydev->addr, AT803X_DEBUG_DATA);
	if (val < 0) {
		val = 0xffff;
		goto out;
	}

	val &= ~clear;
	val |= set;
	bus->write(bus, phydev->addr, AT803X_DEBUG_DATA, val);

out:
	mutex_unlock(&bus->mdio_lock);
	return val;
}

static inline void
at803x_dbg_reg_set(struct phy_device *phydev, u16 reg, u16 set)
{
	at803x_dbg_reg_rmw(phydev, reg, 0, set);
}

static inline void
at803x_dbg_reg_clr(struct phy_device *phydev, u16 reg, u16 clear)
{
	at803x_dbg_reg_rmw(phydev, reg, clear, 0);
}


/* save relevant PHY registers to private copy */
static void at803x_context_save(struct phy_device *phydev,
				struct at803x_context *context)
{
	context->bmcr = phy_read(phydev, MII_BMCR);
	context->advertise = phy_read(phydev, MII_ADVERTISE);
	context->control1000 = phy_read(phydev, MII_CTRL1000);
	context->int_enable = phy_read(phydev, AT803X_INTR_ENABLE);
	context->smart_speed = phy_read(phydev, AT803X_SMART_SPEED);
	context->led_control = phy_read(phydev, AT803X_LED_CONTROL);
}

/* restore relevant PHY registers from private copy */
static void at803x_context_restore(struct phy_device *phydev,
				   const struct at803x_context *context)
{
	phy_write(phydev, MII_BMCR, context->bmcr);
	phy_write(phydev, MII_ADVERTISE, context->advertise);
	phy_write(phydev, MII_CTRL1000, context->control1000);
	phy_write(phydev, AT803X_INTR_ENABLE, context->int_enable);
	phy_write(phydev, AT803X_SMART_SPEED, context->smart_speed);
	phy_write(phydev, AT803X_LED_CONTROL, context->led_control);
}

static int at803x_set_wol(struct phy_device *phydev,
			  struct ethtool_wolinfo *wol)
{
	struct net_device *ndev = phydev->attached_dev;
	const u8 *mac;
	int ret;
	u32 value;
	unsigned int i, offsets[] = {
		AT803X_LOC_MAC_ADDR_32_47_OFFSET,
		AT803X_LOC_MAC_ADDR_16_31_OFFSET,
		AT803X_LOC_MAC_ADDR_0_15_OFFSET,
	};

	if (!ndev)
		return -ENODEV;

	if (wol->wolopts & WAKE_MAGIC) {
		mac = (const u8 *) ndev->dev_addr;

		if (!is_valid_ether_addr(mac))
			return -EINVAL;

		for (i = 0; i < 3; i++) {
			phy_write(phydev, AT803X_MMD_ACCESS_CONTROL,
				  AT803X_DEVICE_ADDR);
			phy_write(phydev, AT803X_MMD_ACCESS_CONTROL_DATA,
				  offsets[i]);
			phy_write(phydev, AT803X_MMD_ACCESS_CONTROL,
				  AT803X_FUNC_DATA);
			phy_write(phydev, AT803X_MMD_ACCESS_CONTROL_DATA,
				  mac[(i * 2) + 1] | (mac[(i * 2)] << 8));
		}

		value = phy_read(phydev, AT803X_INTR_ENABLE);
		value |= AT803X_WOL_ENABLE;
		ret = phy_write(phydev, AT803X_INTR_ENABLE, value);
		if (ret)
			return ret;
		value = phy_read(phydev, AT803X_INTR_STATUS);
	} else {
		value = phy_read(phydev, AT803X_INTR_ENABLE);
		value &= (~AT803X_WOL_ENABLE);
		ret = phy_write(phydev, AT803X_INTR_ENABLE, value);
		if (ret)
			return ret;
		value = phy_read(phydev, AT803X_INTR_STATUS);
	}

	return ret;
}

static void at803x_get_wol(struct phy_device *phydev,
			   struct ethtool_wolinfo *wol)
{
	u32 value;

	wol->supported = WAKE_MAGIC;
	wol->wolopts = 0;

	value = phy_read(phydev, AT803X_INTR_ENABLE);
	if (value & AT803X_WOL_ENABLE)
		wol->wolopts |= WAKE_MAGIC;
}

static int at803x_suspend(struct phy_device *phydev)
{
	int value;
	int wol_enabled;

	mutex_lock(&phydev->lock);

	value = phy_read(phydev, AT803X_INTR_ENABLE);
	wol_enabled = value & AT803X_WOL_ENABLE;

	value = phy_read(phydev, MII_BMCR);

	if (wol_enabled)
		value |= BMCR_ISOLATE;
	else
		value |= BMCR_PDOWN;

	phy_write(phydev, MII_BMCR, value);

	mutex_unlock(&phydev->lock);

	return 0;
}

static int at803x_resume(struct phy_device *phydev)
{
	int value;

	mutex_lock(&phydev->lock);

	value = phy_read(phydev, MII_BMCR);
	value &= ~(BMCR_PDOWN | BMCR_ISOLATE);
	phy_write(phydev, MII_BMCR, value);

	mutex_unlock(&phydev->lock);

	return 0;
}

static int at803x_probe(struct phy_device *phydev)
{
	struct at803x_platform_data *pdata;
	struct device *dev = &phydev->dev;
	struct at803x_priv *priv;
	struct gpio_desc *gpiod_reset;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	if (phydev->drv->phy_id != ATH8030_PHY_ID &&
	    phydev->drv->phy_id != ATH8032_PHY_ID)
		goto does_not_require_reset_workaround;

	pdata = dev_get_platdata(&phydev->dev);
	if (pdata && pdata->has_reset_gpio) {
		devm_gpio_request(dev, pdata->reset_gpio, "reset");
		gpio_direction_output(pdata->reset_gpio, 1);
	}

	gpiod_reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(gpiod_reset))
		return PTR_ERR(gpiod_reset);

	priv->gpiod_reset = gpiod_reset;

does_not_require_reset_workaround:
	phydev->priv = priv;

	return 0;
}

static void at803x_disable_smarteee(struct phy_device *phydev)
{
	phy_write_mmd(phydev, MDIO_MMD_PCS, AT803X_PCS_SMART_EEE_CTRL3,
		1 << AT803X_SMART_EEE_CTRL3_LPI_TX_DELAY_SEL_SHIFT);
	phy_write_mmd(phydev, MDIO_MMD_AN, MDIO_AN_EEE_ADV, 0);
}

static int at803x_phy_reset(struct phy_device *phydev)
{
	int ret = 0, timeout = 0;
	u32 v= 0;

	ret = phy_write(phydev, MII_BMCR, BMCR_RESET);
	if (ret)
	        return -1;

	for (timeout = 0; timeout < 30; timeout ++) {
		v = phy_read(phydev, MII_BMCR);
		if (0 != (v & BMCR_RESET)) {
			mdelay(100);
			continue;
		}
	}
	if (0 != (v & BMCR_RESET)) {
		printk("%s: phy reset fail\n",__func__);
		return -1;
	}
	return 0;
}

static int at803x_config_init(struct phy_device *phydev)
{
	struct at803x_platform_data *pdata;
	int ret;
	u32 v;

	if (phydev->drv->phy_id == ATH8031_PHY_ID &&
		phydev->interface == PHY_INTERFACE_MODE_SGMII)
	{
		v = phy_read(phydev, AT803X_REG_CHIP_CONFIG);
		/* select SGMII/fiber page */
		v &= ~AT803X_BT_BX_REG_SEL;
		ret = phy_write(phydev, AT803X_REG_CHIP_CONFIG, v);
		if (ret)
			return ret;
		//: set copper phy auto_advert
		ret = phy_write(phydev, MII_ADVERTISE, ADVERTISE_ALL |
							ADVERTISE_PAUSE_ASYM |
							ADVERTISE_PAUSE_CAP |
							ADVERTISE_CSMA);
		if (ret)
			return ret;

		//: WAR for ar8033 phy issue
		phy_write(phydev, 0x14, 0x02C);
		//: phy reset first
		ret = at803x_phy_reset(phydev);
		if (ret)
			return ret;

		/* enable SGMII autonegotiation */
		ret = phy_write(phydev, MII_BMCR, BMCR_ANENABLE);
		if (ret)
			return ret;
		/* select copper/priority/SGMII paga */
		ret = phy_write(phydev, AT803X_REG_CHIP_CONFIG,
						v | AT803X_BT_BX_REG_SEL |
						AT803X_PRIORITY_SEL |
						AT803X_MODE_CFG_BASET_SGMII);
		if (ret)
			return ret;

	}

	ret = genphy_config_init(phydev);
	if (ret < 0)
		return ret;

	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID) {
		ret = phy_write(phydev, AT803X_DEBUG_ADDR,
				AT803X_DEBUG_SYSTEM_MODE_CTRL);
		if (ret)
			return ret;
		ret = phy_write(phydev, AT803X_DEBUG_DATA,
				AT803X_DEBUG_RGMII_TX_CLK_DLY);
		if (ret)
			return ret;
	}

	pdata = dev_get_platdata(&phydev->dev);
	if (pdata) {
		if (pdata->disable_smarteee)
			at803x_disable_smarteee(phydev);

		if (pdata->enable_rgmii_rx_delay)
			at803x_dbg_reg_set(phydev, AT803X_DBG0_REG,
				AT803X_DEBUG_RGMII_RX_CLK_DLY);
		else
			at803x_dbg_reg_clr(phydev, AT803X_DBG0_REG,
				AT803X_DEBUG_RGMII_RX_CLK_DLY);

		if (pdata->enable_rgmii_tx_delay)
			at803x_dbg_reg_set(phydev, AT803X_DEBUG_SYSTEM_MODE_CTRL,
				AT803X_DEBUG_RGMII_TX_CLK_DLY);
		else
			at803x_dbg_reg_clr(phydev, AT803X_DEBUG_SYSTEM_MODE_CTRL,
				AT803X_DEBUG_RGMII_TX_CLK_DLY);
	}

	return 0;
}

static int at803x_ack_interrupt(struct phy_device *phydev)
{
	int err;

	err = phy_read(phydev, AT803X_INSR);

	return (err < 0) ? err : 0;
}

static int at803x_config_intr(struct phy_device *phydev)
{
	int err;
	int value;

	value = phy_read(phydev, AT803X_INER);

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, AT803X_INER,
				value | AT803X_INER_INIT);
	else
		err = phy_write(phydev, AT803X_INER, 0);

	return err;
}

static void at803x_send_phy_event(struct phy_device *phydev, int link)
{
	struct phymsg msg;
	struct net_device *ndev = phydev->attached_dev;
	struct net *net = dev_net(ndev);

	msg.port_idx = 0;
	msg.status = link;
	ubnt_net_notify((void *)net, RTNLGRP_LINK, RTM_PHY, (void *)&msg, sizeof(struct phymsg));
}

static void at803x_link_change_notify(struct phy_device *phydev)
{
	struct at803x_priv *priv = phydev->priv;
	struct at803x_platform_data *pdata;
	int value = 0, cur_link = 0;
	static int pre_link = 0;

	value = phy_read(phydev, MII_BMSR);
	cur_link = value & BMSR_LSTATUS;
	if (pre_link != cur_link) {
		pre_link = cur_link;
		at803x_send_phy_event(phydev, pre_link);
	}

	pdata = dev_get_platdata(&phydev->dev);

	/*
	 * Conduct a hardware reset for AT8030 every time a link loss is
	 * signalled. This is necessary to circumvent a hardware bug that
	 * occurs when the cable is unplugged while TX packets are pending
	 * in the FIFO. In such cases, the FIFO enters an error mode it
	 * cannot recover from by software.
	 */
	if (phydev->drv->phy_id == ATH8030_PHY_ID ||
		phydev->drv->phy_id == ATH8032_PHY_ID) {
		if (phydev->state == PHY_NOLINK) {
			if ((priv->gpiod_reset || pdata->has_reset_gpio) &&
				!priv->phy_reset) {
				struct at803x_context context;

				at803x_context_save(phydev, &context);

				if (pdata->has_reset_gpio) {
					gpio_set_value_cansleep(pdata->reset_gpio, 0);
					msleep(1);
					gpio_set_value_cansleep(pdata->reset_gpio, 1);
					msleep(1);
				} else {
					gpiod_set_value(priv->gpiod_reset, 1);
					msleep(1);
					gpiod_set_value(priv->gpiod_reset, 0);
					msleep(1);
				}

				at803x_context_restore(phydev, &context);

				dev_dbg(&phydev->dev, "%s(): phy was reset\n",
					__func__);
				priv->phy_reset = true;
			}
		} else {
			priv->phy_reset = false;
		}
	}

	if (pdata && pdata->fixup_rgmii_tx_delay &&
	    phydev->speed != priv->prev_speed) {
		switch (phydev->speed) {
		case SPEED_10:
		case SPEED_100:
			at803x_dbg_reg_set(phydev,
				AT803X_DEBUG_SYSTEM_MODE_CTRL,
				AT803X_DEBUG_RGMII_TX_CLK_DLY);
			break;
		case SPEED_1000:
			at803x_dbg_reg_clr(phydev,
				AT803X_DEBUG_SYSTEM_MODE_CTRL,
				AT803X_DEBUG_RGMII_TX_CLK_DLY);
			break;
		default:
			break;
		}

		priv->prev_speed = phydev->speed;
	}
}

static struct phy_driver at803x_driver[] = {
{
	/* ATHEROS 8035 */
	.phy_id			= ATH8035_PHY_ID,
	.name			= "Atheros 8035 ethernet",
	.phy_id_mask		= AT803X_PHY_ID_MASK,
	.probe			= at803x_probe,
	.config_init		= at803x_config_init,
	.set_wol		= at803x_set_wol,
	.get_wol		= at803x_get_wol,
	.suspend		= at803x_suspend,
	.resume			= at803x_resume,
	.features		= PHY_GBIT_FEATURES,
	.flags			= PHY_HAS_INTERRUPT,
	.config_aneg		= genphy_config_aneg,
	.read_status		= genphy_read_status,
	.ack_interrupt		= at803x_ack_interrupt,
	.config_intr		= at803x_config_intr,
	.driver			= {
		.owner = THIS_MODULE,
	},
}, {
	/* ATHEROS 8030 */
	.phy_id			= ATH8030_PHY_ID,
	.name			= "Atheros 8030 ethernet",
	.phy_id_mask		= AT803X_PHY_ID_MASK,
	.probe			= at803x_probe,
	.config_init		= at803x_config_init,
	.link_change_notify	= at803x_link_change_notify,
	.set_wol		= at803x_set_wol,
	.get_wol		= at803x_get_wol,
	.suspend		= at803x_suspend,
	.resume			= at803x_resume,
	.features		= PHY_GBIT_FEATURES,
	.flags			= PHY_HAS_INTERRUPT,
	.config_aneg		= genphy_config_aneg,
	.read_status		= genphy_read_status,
	.ack_interrupt		= at803x_ack_interrupt,
	.config_intr		= at803x_config_intr,
	.driver			= {
		.owner = THIS_MODULE,
	},
}, {
	/* ATHEROS 8031 */
	.phy_id			= ATH8031_PHY_ID,
	.name			= "Atheros 8031/8033 ethernet",
	.phy_id_mask		= AT803X_PHY_ID_MASK,
	.probe			= at803x_probe,
	.config_init		= at803x_config_init,
	.link_change_notify	= at803x_link_change_notify,
	.set_wol		= at803x_set_wol,
	.get_wol		= at803x_get_wol,
	.suspend		= at803x_suspend,
	.resume			= at803x_resume,
	.features		= PHY_GBIT_FEATURES,
	.flags			= PHY_HAS_INTERRUPT,
	.config_aneg		= genphy_config_aneg,
	.read_status		= genphy_read_status,
	.ack_interrupt		= &at803x_ack_interrupt,
	.config_intr		= &at803x_config_intr,
	.driver			= {
		.owner = THIS_MODULE,
	},
}, {
	/* ATHEROS 8032 */
	.phy_id			= ATH8032_PHY_ID,
	.name			= "Atheros 8032 ethernet",
	.phy_id_mask		= AT803X_PHY_ID_MASK,
	.probe			= at803x_probe,
	.config_init		= at803x_config_init,
	.link_change_notify	= at803x_link_change_notify,
	.set_wol		= at803x_set_wol,
	.get_wol		= at803x_get_wol,
	.suspend		= at803x_suspend,
	.resume			= at803x_resume,
	.features		= PHY_GBIT_FEATURES,
	.flags			= PHY_HAS_INTERRUPT,
	.config_aneg		= genphy_config_aneg,
	.read_status		= genphy_read_status,
	.ack_interrupt		= &at803x_ack_interrupt,
	.config_intr		= &at803x_config_intr,
	.driver			= {
		.owner = THIS_MODULE,
	},
} };

module_phy_driver(at803x_driver);

static struct mdio_device_id __maybe_unused atheros_tbl[] = {
	{ ATH8030_PHY_ID, AT803X_PHY_ID_MASK },
	{ ATH8031_PHY_ID, AT803X_PHY_ID_MASK },
	{ ATH8032_PHY_ID, AT803X_PHY_ID_MASK },
	{ ATH8035_PHY_ID, AT803X_PHY_ID_MASK },
	{ }
};

MODULE_DEVICE_TABLE(mdio, atheros_tbl);
