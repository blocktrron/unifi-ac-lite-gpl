/*
 * ar8327.c: AR8216 switch driver
 *
 * Copyright (C) 2009 Felix Fietkau <nbd@nbd.name>
 * Copyright (C) 2011-2012 Gabor Juhos <juhosg@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/list.h>
#include <linux/bitops.h>
#include <linux/switch.h>
#include <linux/delay.h>
#include <linux/phy.h>
#include <linux/lockdep.h>
#include <linux/ar8216_platform.h>
#include <linux/workqueue.h>
#include <linux/of_device.h>
#include <linux/leds.h>
#include <linux/mdio.h>

#include "ar8216.h"
#include "ar8327.h"

extern const struct ar8xxx_mib_desc ar8236_mibs[39];
extern const struct switch_attr ar8xxx_sw_attr_vlan[1];

static u32
ar8327_get_pad_cfg(struct ar8327_pad_cfg *cfg)
{
	u32 t;

	if (!cfg)
		return 0;

	t = 0;
	switch (cfg->mode) {
	case AR8327_PAD_NC:
		break;

	case AR8327_PAD_MAC2MAC_MII:
		t = AR8327_PAD_MAC_MII_EN;
		if (cfg->rxclk_sel)
			t |= AR8327_PAD_MAC_MII_RXCLK_SEL;
		if (cfg->txclk_sel)
			t |= AR8327_PAD_MAC_MII_TXCLK_SEL;
		break;

	case AR8327_PAD_MAC2MAC_GMII:
		t = AR8327_PAD_MAC_GMII_EN;
		if (cfg->rxclk_sel)
			t |= AR8327_PAD_MAC_GMII_RXCLK_SEL;
		if (cfg->txclk_sel)
			t |= AR8327_PAD_MAC_GMII_TXCLK_SEL;
		break;

	case AR8327_PAD_MAC_SGMII:
		t = AR8327_PAD_SGMII_EN;

		/*
		 * WAR for the QUalcomm Atheros AP136 board.
		 * It seems that RGMII TX/RX delay settings needs to be
		 * applied for SGMII mode as well, The ethernet is not
		 * reliable without this.
		 */
		t |= cfg->txclk_delay_sel << AR8327_PAD_RGMII_TXCLK_DELAY_SEL_S;
		t |= cfg->rxclk_delay_sel << AR8327_PAD_RGMII_RXCLK_DELAY_SEL_S;
		if (cfg->rxclk_delay_en)
			t |= AR8327_PAD_RGMII_RXCLK_DELAY_EN;
		if (cfg->txclk_delay_en)
			t |= AR8327_PAD_RGMII_TXCLK_DELAY_EN;

		if (cfg->sgmii_delay_en)
			t |= AR8327_PAD_SGMII_DELAY_EN;

		break;

	case AR8327_PAD_MAC2PHY_MII:
		t = AR8327_PAD_PHY_MII_EN;
		if (cfg->rxclk_sel)
			t |= AR8327_PAD_PHY_MII_RXCLK_SEL;
		if (cfg->txclk_sel)
			t |= AR8327_PAD_PHY_MII_TXCLK_SEL;
		break;

	case AR8327_PAD_MAC2PHY_GMII:
		t = AR8327_PAD_PHY_GMII_EN;
		if (cfg->pipe_rxclk_sel)
			t |= AR8327_PAD_PHY_GMII_PIPE_RXCLK_SEL;
		if (cfg->rxclk_sel)
			t |= AR8327_PAD_PHY_GMII_RXCLK_SEL;
		if (cfg->txclk_sel)
			t |= AR8327_PAD_PHY_GMII_TXCLK_SEL;
		break;

	case AR8327_PAD_MAC_RGMII:
		t = AR8327_PAD_RGMII_EN;
		t |= cfg->txclk_delay_sel << AR8327_PAD_RGMII_TXCLK_DELAY_SEL_S;
		t |= cfg->rxclk_delay_sel << AR8327_PAD_RGMII_RXCLK_DELAY_SEL_S;
		if (cfg->rxclk_delay_en)
			t |= AR8327_PAD_RGMII_RXCLK_DELAY_EN;
		if (cfg->txclk_delay_en)
			t |= AR8327_PAD_RGMII_TXCLK_DELAY_EN;
		break;

	case AR8327_PAD_PHY_GMII:
		t = AR8327_PAD_PHYX_GMII_EN;
		break;

	case AR8327_PAD_PHY_RGMII:
		t = AR8327_PAD_PHYX_RGMII_EN;
		break;

	case AR8327_PAD_PHY_MII:
		t = AR8327_PAD_PHYX_MII_EN;
		break;
	}

	return t;
}

static void
ar8327_phy_fixup(struct ar8xxx_priv *priv, int phy)
{
	switch (priv->chip_rev) {
	case 1:
		/* For 100M waveform */
		ar8xxx_phy_dbg_write(priv, phy, 0, 0x02ea);
		/* Turn on Gigabit clock */
		ar8xxx_phy_dbg_write(priv, phy, 0x3d, 0x68a0);
		break;

	case 2:
		ar8xxx_phy_mmd_write(priv, phy, 0x7, 0x3c, 0x0);
		/* fallthrough */
	case 4:
		ar8xxx_phy_mmd_write(priv, phy, 0x3, 0x800d, 0x803f);
		ar8xxx_phy_dbg_write(priv, phy, 0x3d, 0x6860);
		ar8xxx_phy_dbg_write(priv, phy, 0x5, 0x2c46);
		ar8xxx_phy_dbg_write(priv, phy, 0x3c, 0x6000);
		break;
	}
}

static u32
ar8327_get_port_init_status(struct ar8327_port_cfg *cfg)
{
	u32 t;

	if (!cfg->force_link)
		return AR8216_PORT_STATUS_LINK_AUTO;

	t = AR8216_PORT_STATUS_TXMAC | AR8216_PORT_STATUS_RXMAC;
	t |= cfg->duplex ? AR8216_PORT_STATUS_DUPLEX : 0;
	t |= cfg->rxpause ? AR8216_PORT_STATUS_RXFLOW : 0;
	t |= cfg->txpause ? AR8216_PORT_STATUS_TXFLOW : 0;

	switch (cfg->speed) {
	case AR8327_PORT_SPEED_10:
		t |= AR8216_PORT_SPEED_10M;
		break;
	case AR8327_PORT_SPEED_100:
		t |= AR8216_PORT_SPEED_100M;
		break;
	case AR8327_PORT_SPEED_1000:
		t |= AR8216_PORT_SPEED_1000M;
		break;
	}

	return t;
}

#define AR8327_LED_ENTRY(_num, _reg, _shift) \
	[_num] = { .reg = (_reg), .shift = (_shift) }

static const struct ar8327_led_entry
ar8327_led_map[AR8327_NUM_LEDS] = {
	AR8327_LED_ENTRY(AR8327_LED_PHY0_0, 0, 14),
	AR8327_LED_ENTRY(AR8327_LED_PHY0_1, 1, 14),
	AR8327_LED_ENTRY(AR8327_LED_PHY0_2, 2, 14),

	AR8327_LED_ENTRY(AR8327_LED_PHY1_0, 3, 8),
	AR8327_LED_ENTRY(AR8327_LED_PHY1_1, 3, 10),
	AR8327_LED_ENTRY(AR8327_LED_PHY1_2, 3, 12),

	AR8327_LED_ENTRY(AR8327_LED_PHY2_0, 3, 14),
	AR8327_LED_ENTRY(AR8327_LED_PHY2_1, 3, 16),
	AR8327_LED_ENTRY(AR8327_LED_PHY2_2, 3, 18),

	AR8327_LED_ENTRY(AR8327_LED_PHY3_0, 3, 20),
	AR8327_LED_ENTRY(AR8327_LED_PHY3_1, 3, 22),
	AR8327_LED_ENTRY(AR8327_LED_PHY3_2, 3, 24),

	AR8327_LED_ENTRY(AR8327_LED_PHY4_0, 0, 30),
	AR8327_LED_ENTRY(AR8327_LED_PHY4_1, 1, 30),
	AR8327_LED_ENTRY(AR8327_LED_PHY4_2, 2, 30),
};

static void
ar8327_set_led_pattern(struct ar8xxx_priv *priv, unsigned int led_num,
		       enum ar8327_led_pattern pattern)
{
	const struct ar8327_led_entry *entry;

	entry = &ar8327_led_map[led_num];
	ar8xxx_rmw(priv, AR8327_REG_LED_CTRL(entry->reg),
		   (3 << entry->shift), pattern << entry->shift);
}

static void
ar8327_led_work_func(struct work_struct *work)
{
	struct ar8327_led *aled;
	u8 pattern;

	aled = container_of(work, struct ar8327_led, led_work);

	pattern = aled->pattern;

	ar8327_set_led_pattern(aled->sw_priv, aled->led_num,
			       pattern);
}

static void
ar8327_led_schedule_change(struct ar8327_led *aled, u8 pattern)
{
	if (aled->pattern == pattern)
		return;

	aled->pattern = pattern;
	schedule_work(&aled->led_work);
}

static inline struct ar8327_led *
led_cdev_to_ar8327_led(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct ar8327_led, cdev);
}

static int
ar8327_led_blink_set(struct led_classdev *led_cdev,
		     unsigned long *delay_on,
		     unsigned long *delay_off)
{
	struct ar8327_led *aled = led_cdev_to_ar8327_led(led_cdev);

	if (*delay_on == 0 && *delay_off == 0) {
		*delay_on = 125;
		*delay_off = 125;
	}

	if (*delay_on != 125 || *delay_off != 125) {
		/*
		 * The hardware only supports blinking at 4Hz. Fall back
		 * to software implementation in other cases.
		 */
		return -EINVAL;
	}

	spin_lock(&aled->lock);

	aled->enable_hw_mode = false;
	ar8327_led_schedule_change(aled, AR8327_LED_PATTERN_BLINK);

	spin_unlock(&aled->lock);

	return 0;
}

static void
ar8327_led_set_brightness(struct led_classdev *led_cdev,
			  enum led_brightness brightness)
{
	struct ar8327_led *aled = led_cdev_to_ar8327_led(led_cdev);
	u8 pattern;
	bool active;

	active = (brightness != LED_OFF);
	active ^= aled->active_low;

	pattern = (active) ? AR8327_LED_PATTERN_ON :
			     AR8327_LED_PATTERN_OFF;

	spin_lock(&aled->lock);

	aled->enable_hw_mode = false;
	ar8327_led_schedule_change(aled, pattern);

	spin_unlock(&aled->lock);
}

static ssize_t
ar8327_led_enable_hw_mode_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ar8327_led *aled = led_cdev_to_ar8327_led(led_cdev);
	ssize_t ret = 0;

	ret += scnprintf(buf, PAGE_SIZE, "%d\n", aled->enable_hw_mode);

	return ret;
}

static ssize_t
ar8327_led_enable_hw_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t size)
{
        struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ar8327_led *aled = led_cdev_to_ar8327_led(led_cdev);
	u8 pattern;
	u8 value;
	int ret;

	ret = kstrtou8(buf, 10, &value);
	if (ret < 0)
		return -EINVAL;

	spin_lock(&aled->lock);

	aled->enable_hw_mode = !!value;
	if (aled->enable_hw_mode)
		pattern = AR8327_LED_PATTERN_RULE;
	else
		pattern = AR8327_LED_PATTERN_OFF;

	ar8327_led_schedule_change(aled, pattern);

	spin_unlock(&aled->lock);

	return size;
}

static DEVICE_ATTR(enable_hw_mode,  S_IRUGO | S_IWUSR,
		   ar8327_led_enable_hw_mode_show,
		   ar8327_led_enable_hw_mode_store);

static int
ar8327_led_register(struct ar8327_led *aled)
{
	int ret;

	ret = led_classdev_register(NULL, &aled->cdev);
	if (ret < 0)
		return ret;

	if (aled->mode == AR8327_LED_MODE_HW) {
		ret = device_create_file(aled->cdev.dev,
					 &dev_attr_enable_hw_mode);
		if (ret)
			goto err_unregister;
	}

	return 0;

err_unregister:
	led_classdev_unregister(&aled->cdev);
	return ret;
}

static void
ar8327_led_unregister(struct ar8327_led *aled)
{
	if (aled->mode == AR8327_LED_MODE_HW)
		device_remove_file(aled->cdev.dev, &dev_attr_enable_hw_mode);

	led_classdev_unregister(&aled->cdev);
	cancel_work_sync(&aled->led_work);
}

static int
ar8327_led_create(struct ar8xxx_priv *priv,
		  const struct ar8327_led_info *led_info)
{
	struct ar8327_data *data = priv->chip_data;
	struct ar8327_led *aled;
	int ret;

	if (!IS_ENABLED(CONFIG_AR8216_PHY_LEDS))
		return 0;

	if (!led_info->name)
		return -EINVAL;

	if (led_info->led_num >= AR8327_NUM_LEDS)
		return -EINVAL;

	aled = kzalloc(sizeof(*aled) + strlen(led_info->name) + 1,
		       GFP_KERNEL);
	if (!aled)
		return -ENOMEM;

	aled->sw_priv = priv;
	aled->led_num = led_info->led_num;
	aled->active_low = led_info->active_low;
	aled->mode = led_info->mode;

	if (aled->mode == AR8327_LED_MODE_HW)
		aled->enable_hw_mode = true;

	aled->name = (char *)(aled + 1);
	strcpy(aled->name, led_info->name);

	aled->cdev.name = aled->name;
	aled->cdev.brightness_set = ar8327_led_set_brightness;
	aled->cdev.blink_set = ar8327_led_blink_set;
	aled->cdev.default_trigger = led_info->default_trigger;

	spin_lock_init(&aled->lock);
	mutex_init(&aled->mutex);
	INIT_WORK(&aled->led_work, ar8327_led_work_func);

	ret = ar8327_led_register(aled);
	if (ret)
		goto err_free;

	data->leds[data->num_leds++] = aled;

	return 0;

err_free:
	kfree(aled);
	return ret;
}

static void
ar8327_led_destroy(struct ar8327_led *aled)
{
	ar8327_led_unregister(aled);
	kfree(aled);
}

static void
ar8327_leds_init(struct ar8xxx_priv *priv)
{
	struct ar8327_data *data = priv->chip_data;
	unsigned i;

	if (!IS_ENABLED(CONFIG_AR8216_PHY_LEDS))
		return;

	for (i = 0; i < data->num_leds; i++) {
		struct ar8327_led *aled;

		aled = data->leds[i];

		if (aled->enable_hw_mode)
			aled->pattern = AR8327_LED_PATTERN_RULE;
		else
			aled->pattern = AR8327_LED_PATTERN_OFF;

		ar8327_set_led_pattern(priv, aled->led_num, aled->pattern);
	}
}

static void
ar8327_leds_cleanup(struct ar8xxx_priv *priv)
{
	struct ar8327_data *data = priv->chip_data;
	unsigned i;

	if (!IS_ENABLED(CONFIG_AR8216_PHY_LEDS))
		return;

	for (i = 0; i < data->num_leds; i++) {
		struct ar8327_led *aled;

		aled = data->leds[i];
		ar8327_led_destroy(aled);
	}

	kfree(data->leds);
}

static int
ar8327_hw_config_pdata(struct ar8xxx_priv *priv,
		       struct ar8327_platform_data *pdata)
{
	struct ar8327_led_cfg *led_cfg;
	struct ar8327_data *data = priv->chip_data;
	u32 pos, new_pos;
	u32 t;

	if (!pdata)
		return -EINVAL;

	priv->get_port_link = pdata->get_port_link;

	data->port0_status = ar8327_get_port_init_status(&pdata->port0_cfg);
	data->port6_status = ar8327_get_port_init_status(&pdata->port6_cfg);

	t = ar8327_get_pad_cfg(pdata->pad0_cfg);
	if (chip_is_ar8337(priv) && !pdata->pad0_cfg->mac06_exchange_dis)
	    t |= AR8337_PAD_MAC06_EXCHANGE_EN;
	ar8xxx_write(priv, AR8327_REG_PAD0_MODE, t);

	t = ar8327_get_pad_cfg(pdata->pad5_cfg);
	if (chip_is_ar8337(priv)) {
		/*
		 * Workaround: RGMII RX delay setting needs to be
		 * always specified for AR8337 to avoid port 5
		 * RX hang on high traffic / flood conditions
		 */
		t |= AR8327_PAD_RGMII_RXCLK_DELAY_EN;
	}
	ar8xxx_write(priv, AR8327_REG_PAD5_MODE, t);
	t = ar8327_get_pad_cfg(pdata->pad6_cfg);
	ar8xxx_write(priv, AR8327_REG_PAD6_MODE, t);

	pos = ar8xxx_read(priv, AR8327_REG_POWER_ON_STRIP);
	new_pos = pos;

	led_cfg = pdata->led_cfg;
	if (led_cfg) {
		if (led_cfg->open_drain)
			new_pos |= AR8327_POWER_ON_STRIP_LED_OPEN_EN;
		else
			new_pos &= ~AR8327_POWER_ON_STRIP_LED_OPEN_EN;

		ar8xxx_write(priv, AR8327_REG_LED_CTRL0, led_cfg->led_ctrl0);
		ar8xxx_write(priv, AR8327_REG_LED_CTRL1, led_cfg->led_ctrl1);
		ar8xxx_write(priv, AR8327_REG_LED_CTRL2, led_cfg->led_ctrl2);
		ar8xxx_write(priv, AR8327_REG_LED_CTRL3, led_cfg->led_ctrl3);

		if (new_pos != pos)
			new_pos |= AR8327_POWER_ON_STRIP_POWER_ON_SEL;
	}

	if (pdata->sgmii_cfg) {
		t = pdata->sgmii_cfg->sgmii_ctrl;
		if (priv->chip_rev == 1)
			t |= AR8327_SGMII_CTRL_EN_PLL |
			     AR8327_SGMII_CTRL_EN_RX |
			     AR8327_SGMII_CTRL_EN_TX;
		else
			t &= ~(AR8327_SGMII_CTRL_EN_PLL |
			       AR8327_SGMII_CTRL_EN_RX |
			       AR8327_SGMII_CTRL_EN_TX);

		ar8xxx_write(priv, AR8327_REG_SGMII_CTRL, t);

		if (pdata->sgmii_cfg->serdes_aen)
			new_pos &= ~AR8327_POWER_ON_STRIP_SERDES_AEN;
		else
			new_pos |= AR8327_POWER_ON_STRIP_SERDES_AEN;
	}

	ar8xxx_write(priv, AR8327_REG_POWER_ON_STRIP, new_pos);

	if (pdata->leds && pdata->num_leds) {
		int i;

		data->leds = kzalloc(pdata->num_leds * sizeof(void *),
				     GFP_KERNEL);
		if (!data->leds)
			return -ENOMEM;

		for (i = 0; i < pdata->num_leds; i++)
			ar8327_led_create(priv, &pdata->leds[i]);
	}

	return 0;
}

#ifdef CONFIG_OF
static int
ar8327_hw_config_of(struct ar8xxx_priv *priv, struct device_node *np)
{
	struct ar8327_data *data = priv->chip_data;
	const __be32 *paddr;
	int len;
	int i;

	paddr = of_get_property(np, "qca,ar8327-initvals", &len);
	if (!paddr || len < (2 * sizeof(*paddr)))
		return -EINVAL;

	len /= sizeof(*paddr);

	for (i = 0; i < len - 1; i += 2) {
		u32 reg;
		u32 val;

		reg = be32_to_cpup(paddr + i);
		val = be32_to_cpup(paddr + i + 1);

		switch (reg) {
		case AR8327_REG_PORT_STATUS(0):
			data->port0_status = val;
			break;
		case AR8327_REG_PORT_STATUS(6):
			data->port6_status = val;
			break;
		default:
			ar8xxx_write(priv, reg, val);
			break;
		}
	}

	return 0;
}
#else
static inline int
ar8327_hw_config_of(struct ar8xxx_priv *priv, struct device_node *np)
{
	return -EINVAL;
}
#endif

static int
ar8327_hw_init(struct ar8xxx_priv *priv)
{
	int ret;

	priv->chip_data = kzalloc(sizeof(struct ar8327_data), GFP_KERNEL);
	if (!priv->chip_data)
		return -ENOMEM;

	if (priv->phy->dev.of_node)
		ret = ar8327_hw_config_of(priv, priv->phy->dev.of_node);
	else
		ret = ar8327_hw_config_pdata(priv,
					     priv->phy->dev.platform_data);

	if (ret)
		return ret;

	ar8327_leds_init(priv);

	ar8xxx_phy_init(priv);

	return 0;
}

static void
ar8327_cleanup(struct ar8xxx_priv *priv)
{
	ar8327_leds_cleanup(priv);
}

static void
ar8327_init_globals(struct ar8xxx_priv *priv)
{
	struct ar8327_data *data = priv->chip_data;
	u32 t;
	int i;

	/* enable CPU port and disable mirror port */
	t = AR8327_FWD_CTRL0_CPU_PORT_EN |
	    AR8327_FWD_CTRL0_MIRROR_PORT;
	ar8xxx_write(priv, AR8327_REG_FWD_CTRL0, t);

	/* forward multicast and broadcast frames to CPU */
	t = (AR8327_PORTS_ALL << AR8327_FWD_CTRL1_UC_FLOOD_S) |
	    (AR8327_PORTS_ALL << AR8327_FWD_CTRL1_MC_FLOOD_S) |
	    (AR8327_PORTS_ALL << AR8327_FWD_CTRL1_BC_FLOOD_S);
	ar8xxx_write(priv, AR8327_REG_FWD_CTRL1, t);

	/* enable jumbo frames */
	ar8xxx_rmw(priv, AR8327_REG_MAX_FRAME_SIZE,
		   AR8327_MAX_FRAME_SIZE_MTU, 9018 + 8 + 2);

	/* Enable MIB counters */
	ar8xxx_reg_set(priv, AR8327_REG_MODULE_EN,
		       AR8327_MODULE_EN_MIB);

	/* Enable ACL module */
	ar8xxx_reg_set(priv, AR8327_REG_MODULE_EN,
		       AR8327_MODULE_EN_ACL);

	/* Disable EEE on all phy's due to stability issues */
	for (i = 0; i < AR8XXX_NUM_PHYS; i++)
		data->eee[i] = false;

	for (i = 0; i < AR8X16_MAX_PORTS; i++) {
		priv->port_ingress_mode_override[i] = false;
		priv->port_egress_mode_override[i] = false;
		priv->port_qos_egress_cir[i] = 0x7FFF;
		priv->port_qos_egress_eir[i] = 0x7FFF;
		priv->port_stp_state[i] = AR8216_PORT_STATE_FORWARD;
	}

	if (chip_is_ar8337(priv)) {
		/* Update HOL registers with values suggested by QCA switch team */
		for (i = 0; i < AR8327_NUM_PORTS; i++) {
			if (i == AR8216_PORT_CPU || i == 5 || i == 6) {
				t = 0x3 << AR8327_PORT_HOL_CTRL0_EG_PRI0_BUF_S;
				t |= 0x4 << AR8327_PORT_HOL_CTRL0_EG_PRI1_BUF_S;
				t |= 0x4 << AR8327_PORT_HOL_CTRL0_EG_PRI2_BUF_S;
				t |= 0x4 << AR8327_PORT_HOL_CTRL0_EG_PRI3_BUF_S;
				t |= 0x6 << AR8327_PORT_HOL_CTRL0_EG_PRI4_BUF_S;
				t |= 0x8 << AR8327_PORT_HOL_CTRL0_EG_PRI5_BUF_S;
				t |= 0x1e << AR8327_PORT_HOL_CTRL0_EG_PORT_BUF_S;
			} else {
				t = 0x3 << AR8327_PORT_HOL_CTRL0_EG_PRI0_BUF_S;
				t |= 0x4 << AR8327_PORT_HOL_CTRL0_EG_PRI1_BUF_S;
				t |= 0x6 << AR8327_PORT_HOL_CTRL0_EG_PRI2_BUF_S;
				t |= 0x8 << AR8327_PORT_HOL_CTRL0_EG_PRI3_BUF_S;
				t |= 0x19 << AR8327_PORT_HOL_CTRL0_EG_PORT_BUF_S;
			}
			ar8xxx_write(priv, AR8327_REG_PORT_HOL_CTRL0(i), t);

			t = 0x6 << AR8327_PORT_HOL_CTRL1_ING_BUF_S;
			t |= AR8327_PORT_HOL_CTRL1_EG_PRI_BUF_EN;
			t |= AR8327_PORT_HOL_CTRL1_EG_PORT_BUF_EN;
			t |= AR8327_PORT_HOL_CTRL1_WRED_EN;
			ar8xxx_rmw(priv, AR8327_REG_PORT_HOL_CTRL1(i),
				   AR8327_PORT_HOL_CTRL1_ING_BUF |
				   AR8327_PORT_HOL_CTRL1_EG_PRI_BUF_EN |
				   AR8327_PORT_HOL_CTRL1_EG_PORT_BUF_EN |
				   AR8327_PORT_HOL_CTRL1_WRED_EN,
				   t);
		}
	}
}

static void
ar8327_init_port(struct ar8xxx_priv *priv, int port)
{
	struct ar8327_data *data = priv->chip_data;
	u32 t;

	if (port == AR8216_PORT_CPU)
		t = data->port0_status;
	else if (port == 6)
		t = data->port6_status;
	else
		t = AR8216_PORT_STATUS_LINK_AUTO;

	if (port != AR8216_PORT_CPU && port != 6) {
		/*hw limitation:if configure mac when there is traffic,
		port MAC may work abnormal. Need disable lan&wan mac at fisrt*/
		ar8xxx_write(priv, AR8327_REG_PORT_STATUS(port), 0);
		msleep(100);
		t |= AR8216_PORT_STATUS_FLOW_CONTROL;
		ar8xxx_write(priv, AR8327_REG_PORT_STATUS(port), t);
	} else {
		ar8xxx_write(priv, AR8327_REG_PORT_STATUS(port), t);
	}

	ar8xxx_write(priv, AR8327_REG_PORT_HEADER(port), 0);

	ar8xxx_write(priv, AR8327_REG_PORT_VLAN0(port), 0);

	t = AR8327_PORT_VLAN1_OUT_MODE_UNTOUCH << AR8327_PORT_VLAN1_OUT_MODE_S;
	ar8xxx_write(priv, AR8327_REG_PORT_VLAN1(port), t);

	t = AR8327_PORT_LOOKUP_LEARN;
	if (priv->vlan)
		t |= AR8327_FORCE_PORT_VLAN_EN;
	t |= AR8216_PORT_STATE_FORWARD << AR8327_PORT_LOOKUP_STATE_S;
	ar8xxx_write(priv, AR8327_REG_PORT_LOOKUP(port), t);
}

static u32
ar8327_read_port_status(struct ar8xxx_priv *priv, int port)
{
	u32 t;

	t = ar8xxx_read(priv, AR8327_REG_PORT_STATUS(port));
	/* map the flow control autoneg result bits to the flow control bits
	 * used in forced mode to allow ar8216_read_port_link detect
	 * flow control properly if autoneg is used
	 */
	if (t & AR8216_PORT_STATUS_LINK_UP &&
	    t & AR8216_PORT_STATUS_LINK_AUTO) {
		t &= ~(AR8216_PORT_STATUS_TXFLOW | AR8216_PORT_STATUS_RXFLOW);
		if (t & AR8327_PORT_STATUS_TXFLOW_AUTO)
			t |= AR8216_PORT_STATUS_TXFLOW;
		if (t & AR8327_PORT_STATUS_RXFLOW_AUTO)
			t |= AR8216_PORT_STATUS_RXFLOW;
	}

	return t;
}

static u32
ar8327_read_port_eee_status(struct ar8xxx_priv *priv, int port)
{
	int phy;
	u16 t;

	if (port >= priv->dev.ports)
		return 0;

	if (port == 0 || port == 6)
		return 0;

	phy = port - 1;

	/* EEE Ability Auto-negotiation Result */
	t = ar8xxx_phy_mmd_read(priv, phy, 0x7, 0x8000);

	return mmd_eee_adv_to_ethtool_adv_t(t);
}

static int
ar8327_atu_flush(struct ar8xxx_priv *priv)
{
	int ret;

	ret = ar8216_wait_bit(priv, AR8327_REG_ATU_FUNC,
			      AR8327_ATU_FUNC_BUSY, 0);
	if (!ret)
		ar8xxx_write(priv, AR8327_REG_ATU_FUNC,
			     AR8327_ATU_FUNC_OP_FLUSH |
			     AR8327_ATU_FUNC_BUSY);

	return ret;
}

static int
ar8327_atu_flush_port(struct ar8xxx_priv *priv, int port)
{
	u32 t;
	int ret;

	ret = ar8216_wait_bit(priv, AR8327_REG_ATU_FUNC,
			      AR8327_ATU_FUNC_BUSY, 0);
	if (!ret) {
		t = (port << AR8327_ATU_PORT_NUM_S);
		t |= AR8327_ATU_FUNC_OP_FLUSH_PORT;
		t |= AR8327_ATU_FUNC_BUSY;
		ar8xxx_write(priv, AR8327_REG_ATU_FUNC, t);
	}

	return ret;
}

static int
ar8327_get_port_igmp(struct ar8xxx_priv *priv, int port)
{
	u32 fwd_ctrl, frame_ack;

	fwd_ctrl = (BIT(port) << AR8327_FWD_CTRL1_IGMP_S);
	frame_ack = ((AR8327_FRAME_ACK_CTRL_IGMP_MLD |
		      AR8327_FRAME_ACK_CTRL_IGMP_JOIN |
		      AR8327_FRAME_ACK_CTRL_IGMP_LEAVE) <<
		     AR8327_FRAME_ACK_CTRL_S(port));

	return (ar8xxx_read(priv, AR8327_REG_FWD_CTRL1) &
			fwd_ctrl) == fwd_ctrl &&
		(ar8xxx_read(priv, AR8327_REG_FRAME_ACK_CTRL(port)) &
			frame_ack) == frame_ack;
}

static void
ar8327_set_port_igmp(struct ar8xxx_priv *priv, int port, int enable)
{
	int reg_frame_ack = AR8327_REG_FRAME_ACK_CTRL(port);
	u32 val_frame_ack = (AR8327_FRAME_ACK_CTRL_IGMP_MLD |
			  AR8327_FRAME_ACK_CTRL_IGMP_JOIN |
			  AR8327_FRAME_ACK_CTRL_IGMP_LEAVE) <<
			 AR8327_FRAME_ACK_CTRL_S(port);

	if (enable) {
		ar8xxx_rmw(priv, AR8327_REG_FWD_CTRL1,
			   BIT(port) << AR8327_FWD_CTRL1_MC_FLOOD_S,
			   BIT(port) << AR8327_FWD_CTRL1_IGMP_S);
		ar8xxx_reg_set(priv, reg_frame_ack, val_frame_ack);
	} else {
		ar8xxx_rmw(priv, AR8327_REG_FWD_CTRL1,
			   BIT(port) << AR8327_FWD_CTRL1_IGMP_S,
			   BIT(port) << AR8327_FWD_CTRL1_MC_FLOOD_S);
		ar8xxx_reg_clear(priv, reg_frame_ack, val_frame_ack);
	}
}

static void
ar8327_vtu_op(struct ar8xxx_priv *priv, u32 op, u32 val)
{
	if (ar8216_wait_bit(priv, AR8327_REG_VTU_FUNC1,
			    AR8327_VTU_FUNC1_BUSY, 0))
		return;

	if ((op & AR8327_VTU_FUNC1_OP) == AR8327_VTU_FUNC1_OP_LOAD)
		ar8xxx_write(priv, AR8327_REG_VTU_FUNC0, val);

	op |= AR8327_VTU_FUNC1_BUSY;
	ar8xxx_write(priv, AR8327_REG_VTU_FUNC1, op);
}

static void
ar8327_vtu_flush(struct ar8xxx_priv *priv)
{
	ar8327_vtu_op(priv, AR8327_VTU_FUNC1_OP_FLUSH, 0);
}

static void
ar8327_vtu_load_vlan(struct ar8xxx_priv *priv, u32 vid, u32 port_mask)
{
	u32 op;
	u32 val;
	int i;

	op = AR8327_VTU_FUNC1_OP_LOAD | (vid << AR8327_VTU_FUNC1_VID_S);
	val = AR8327_VTU_FUNC0_VALID | AR8327_VTU_FUNC0_IVL;
	for (i = 0; i < AR8327_NUM_PORTS; i++) {
		u32 mode;

		if ((port_mask & BIT(i)) == 0)
			mode = AR8327_VTU_FUNC0_EG_MODE_NOT;
		else if (priv->vlan == 0)
			mode = AR8327_VTU_FUNC0_EG_MODE_KEEP;
		else if ((priv->vlan_tagged & BIT(i)) || (priv->vlan_id[priv->pvid[i]] != vid))
			mode = AR8327_VTU_FUNC0_EG_MODE_TAG;
		else
			mode = AR8327_VTU_FUNC0_EG_MODE_UNTAG;

		val |= mode << AR8327_VTU_FUNC0_EG_MODE_S(i);
	}
	ar8327_vtu_op(priv, op, val);
}

static void
ar8327_setup_port(struct ar8xxx_priv *priv, int port, u32 members)
{
	u32 t;
	u32 egress, ingress;
	u32 pvid = priv->vlan_id[priv->pvid[port]];

	if (priv->vlan) {
		if(priv->port_egress_mode_override[port])
			egress = priv->port_egress_mode_override_val[port];
		else
			egress = AR8327_PORT_VLAN1_OUT_MODE_UNMOD;
		if(priv->port_ingress_mode_override[port])
			ingress = priv->port_ingress_mode_override_val[port];
		else
			ingress = AR8216_IN_SECURE;
	} else {
		egress = AR8327_PORT_VLAN1_OUT_MODE_UNTOUCH;
		ingress = AR8216_IN_PORT_ONLY;
	}

	t = pvid << AR8327_PORT_VLAN0_DEF_SVID_S;
	t |= pvid << AR8327_PORT_VLAN0_DEF_CVID_S;
	ar8xxx_write(priv, AR8327_REG_PORT_VLAN0(port), t);

	t = AR8327_PORT_VLAN1_PORT_VLAN_PROP;
	t |= egress << AR8327_PORT_VLAN1_OUT_MODE_S;
	ar8xxx_write(priv, AR8327_REG_PORT_VLAN1(port), t);

	t = members;
	t |= AR8327_PORT_LOOKUP_LEARN;
	t |= ingress << AR8327_PORT_LOOKUP_IN_MODE_S;
	t |= priv->port_stp_state[port] << AR8327_PORT_LOOKUP_STATE_S;
	ar8xxx_write(priv, AR8327_REG_PORT_LOOKUP(port), t);

	// Set up per-port egress shaping

	// For now, let's not support setting per-priority shaping and just set the same limit for all priorities

	// Committed Information Rate
	t = priv->port_qos_egress_cir[port] << AR8327_EG_PRI0_CIR_S;
	t |= priv->port_qos_egress_cir[port] << AR8327_EG_PRI1_CIR_S;
	ar8xxx_write(priv, AR8327_REG_PORT_EG_RATE_CTRL0(port), t);

	t = priv->port_qos_egress_cir[port] << AR8327_EG_PRI2_CIR_S;
	t |= priv->port_qos_egress_cir[port] << AR8327_EG_PRI3_CIR_S;
	ar8xxx_write(priv, AR8327_REG_PORT_EG_RATE_CTRL1(port), t);

	t = priv->port_qos_egress_cir[port] << AR8327_EG_PRI4_CIR_S;
	t |= priv->port_qos_egress_cir[port] << AR8327_EG_PRI5_CIR_S;
	ar8xxx_write(priv, AR8327_REG_PORT_EG_RATE_CTRL2(port), t);

	// Excess Information Rate
	t = priv->port_qos_egress_eir[port] << AR8327_EG_PRI0_EIR_S;
	t |= priv->port_qos_egress_eir[port] << AR8327_EG_PRI1_EIR_S;
	ar8xxx_write(priv, AR8327_REG_PORT_EG_RATE_CTRL3(port), t);

	t = priv->port_qos_egress_eir[port] << AR8327_EG_PRI2_EIR_S;
	t |= priv->port_qos_egress_eir[port] << AR8327_EG_PRI3_EIR_S;
	ar8xxx_write(priv, AR8327_REG_PORT_EG_RATE_CTRL4(port), t);

	t = priv->port_qos_egress_eir[port] << AR8327_EG_PRI4_EIR_S;
	t |= priv->port_qos_egress_eir[port] << AR8327_EG_PRI5_EIR_S;
	ar8xxx_write(priv, AR8327_REG_PORT_EG_RATE_CTRL5(port), t);
}

static int
ar8327_sw_get_ports(struct switch_dev *dev, struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	u8 ports = priv->vlan_table[val->port_vlan];
	int i;

	val->len = 0;
	for (i = 0; i < dev->ports; i++) {
		struct switch_port *p;

		if (!(ports & (1 << i)))
			continue;

		p = &val->value.ports[val->len++];
		p->id = i;
		if ((priv->vlan_tagged & (1 << i)) || (priv->pvid[i] != val->port_vlan))
			p->flags = (1 << SWITCH_PORT_FLAG_TAGGED);
		else
			p->flags = 0;
	}
	return 0;
}

static int
ar8327_sw_set_ports(struct switch_dev *dev, struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	u8 *vt = &priv->vlan_table[val->port_vlan];
	int i;

	*vt = 0;
	for (i = 0; i < val->len; i++) {
		struct switch_port *p = &val->value.ports[i];

		if (p->flags & (1 << SWITCH_PORT_FLAG_TAGGED)) {
			if (val->port_vlan == priv->pvid[p->id]) {
				priv->vlan_tagged |= (1 << p->id);
			}
		} else {
			priv->vlan_tagged &= ~(1 << p->id);
			priv->pvid[p->id] = val->port_vlan;
		}

		*vt |= 1 << p->id;
	}
	return 0;
}

static void
ar8327_set_mirror_regs(struct ar8xxx_priv *priv)
{
	int port;

	/* reset all mirror registers */
	ar8xxx_rmw(priv, AR8327_REG_FWD_CTRL0,
		   AR8327_FWD_CTRL0_MIRROR_PORT,
		   (0xF << AR8327_FWD_CTRL0_MIRROR_PORT_S));
	for (port = 0; port < AR8327_NUM_PORTS; port++) {
		ar8xxx_reg_clear(priv, AR8327_REG_PORT_LOOKUP(port),
			   AR8327_PORT_LOOKUP_ING_MIRROR_EN);

		ar8xxx_reg_clear(priv, AR8327_REG_PORT_HOL_CTRL1(port),
			   AR8327_PORT_HOL_CTRL1_EG_MIRROR_EN);
	}

	/* now enable mirroring if necessary */
	if (priv->source_port >= AR8327_NUM_PORTS ||
	    priv->monitor_port >= AR8327_NUM_PORTS ||
	    priv->source_port == priv->monitor_port) {
		return;
	}

	ar8xxx_rmw(priv, AR8327_REG_FWD_CTRL0,
		   AR8327_FWD_CTRL0_MIRROR_PORT,
		   (priv->monitor_port << AR8327_FWD_CTRL0_MIRROR_PORT_S));

	if (priv->mirror_rx)
		ar8xxx_reg_set(priv, AR8327_REG_PORT_LOOKUP(priv->source_port),
			   AR8327_PORT_LOOKUP_ING_MIRROR_EN);

	if (priv->mirror_tx)
		ar8xxx_reg_set(priv, AR8327_REG_PORT_HOL_CTRL1(priv->source_port),
			   AR8327_PORT_HOL_CTRL1_EG_MIRROR_EN);
}

static int
ar8327_sw_set_eee(struct switch_dev *dev,
		  const struct switch_attr *attr,
		  struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	struct ar8327_data *data = priv->chip_data;
	int port = val->port_vlan;
	int phy;

	if (port >= dev->ports)
		return -EINVAL;
	if (port == 0 || port == 6)
		return -EOPNOTSUPP;

	phy = port - 1;

	data->eee[phy] = !!(val->value.i);

	return 0;
}

static int
ar8327_sw_get_eee(struct switch_dev *dev,
		  const struct switch_attr *attr,
		  struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	const struct ar8327_data *data = priv->chip_data;
	int port = val->port_vlan;
	int phy;

	if (port >= dev->ports)
		return -EINVAL;
	if (port == 0 || port == 6)
		return -EOPNOTSUPP;

	phy = port - 1;

	val->value.i = data->eee[phy];

	return 0;
}

static void
ar8327_wait_atu_ready(struct ar8xxx_priv *priv, u16 r2, u16 r1)
{
	int timeout = 20;

	while (ar8xxx_mii_read32(priv, r2, r1) & AR8327_ATU_FUNC_BUSY && --timeout)
                usleep_range(10, 20);

	if (!timeout)
		pr_err("ar8327: timeout waiting for atu to become ready\n");
}

static void ar8327_get_arl_entry(struct ar8xxx_priv *priv,
				 struct arl_read_tmp_entry *a, u32 *status, enum arl_op op)
{
	struct mii_bus *bus = priv->mii_bus;
	u16 r2, page;
	u16 r1_data0, r1_data1, r1_data2, r1_func;
	u32 t, val0, val1, val2;
	int i;

	split_addr(AR8327_REG_ATU_DATA0, &r1_data0, &r2, &page);
	r2 |= 0x10;

	r1_data1 = (AR8327_REG_ATU_DATA1 >> 1) & 0x1e;
	r1_data2 = (AR8327_REG_ATU_DATA2 >> 1) & 0x1e;
	r1_func  = (AR8327_REG_ATU_FUNC >> 1) & 0x1e;

	switch (op) {
	case AR8XXX_ARL_INITIALIZE:
		/* all ATU registers are on the same page
		* therefore set page only once
		*/
		bus->write(bus, 0x18, 0, page);
		wait_for_page_switch();

		ar8327_wait_atu_ready(priv, r2, r1_func);

		ar8xxx_mii_write32(priv, r2, r1_data0, 0);
		ar8xxx_mii_write32(priv, r2, r1_data1, 0);
		ar8xxx_mii_write32(priv, r2, r1_data2, 0);
		break;
	case AR8XXX_ARL_GET_NEXT:
		ar8xxx_mii_write32(priv, r2, r1_func,
				   AR8327_ATU_FUNC_OP_GET_NEXT |
				   AR8327_ATU_FUNC_BUSY);
		ar8327_wait_atu_ready(priv, r2, r1_func);

		val0 = ar8xxx_mii_read32(priv, r2, r1_data0);
		val1 = ar8xxx_mii_read32(priv, r2, r1_data1);
		val2 = ar8xxx_mii_read32(priv, r2, r1_data2);

		*status = val2 & AR8327_ATU_STATUS;
		if (!*status)
			break;

		i = 0;
		t = AR8327_ATU_PORT0;
		while (!(val1 & t) && ++i < AR8327_NUM_PORTS)
			t <<= 1;

		a->port = i;
		a->mac[0] = (val0 & AR8327_ATU_ADDR0) >> AR8327_ATU_ADDR0_S;
		a->mac[1] = (val0 & AR8327_ATU_ADDR1) >> AR8327_ATU_ADDR1_S;
		a->mac[2] = (val0 & AR8327_ATU_ADDR2) >> AR8327_ATU_ADDR2_S;
		a->mac[3] = (val0 & AR8327_ATU_ADDR3) >> AR8327_ATU_ADDR3_S;
		a->mac[4] = (val1 & AR8327_ATU_ADDR4) >> AR8327_ATU_ADDR4_S;
		a->mac[5] = (val1 & AR8327_ATU_ADDR5) >> AR8327_ATU_ADDR5_S;
		a->vid = (val2 & AR8327_ATU_VID) >> AR8327_ATU_VID_S;
		break;
	}
}

static int
ar8327_sw_hw_apply(struct switch_dev *dev)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	const struct ar8327_data *data = priv->chip_data;
	int ret, i;

	ret = ar8xxx_sw_hw_apply(dev);
	if (ret)
		return ret;

	for (i=0; i < AR8XXX_NUM_PHYS; i++) {
		if (data->eee[i])
			ar8xxx_reg_clear(priv, AR8327_REG_EEE_CTRL,
			       AR8327_EEE_CTRL_DISABLE_PHY(i));
		else
			ar8xxx_reg_set(priv, AR8327_REG_EEE_CTRL,
			       AR8327_EEE_CTRL_DISABLE_PHY(i));
	}

	return 0;
}

static int
ar8327_sw_set_port_ingress_mode(struct switch_dev *dev,
				const struct switch_attr *attr,
				struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	priv->port_ingress_mode_override_val[port] = val->value.i & 3;

	return 0;
}

int
ar8327_sw_get_port_ingress_mode(struct switch_dev *dev,
				const struct switch_attr *attr,
				struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	val->value.i = priv->port_ingress_mode_override_val[port];

	return 0;
}

int
ar8327_sw_set_port_egress_mode(struct switch_dev *dev,
					 const struct switch_attr *attr,
					 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	priv->port_egress_mode_override_val[port] = val->value.i & 3;

	return 0;
}

int
ar8327_sw_get_port_egress_mode(struct switch_dev *dev,
					 const struct switch_attr *attr,
					 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	val->value.i = priv->port_egress_mode_override_val[port];

	return 0;
}

int
ar8327_sw_set_port_ingress_mode_override(struct switch_dev *dev,
					 const struct switch_attr *attr,
					 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	priv->port_ingress_mode_override[port] = (val->value.i ? true : false);

	return 0;
}

int
ar8327_sw_get_port_ingress_mode_override(struct switch_dev *dev,
					 const struct switch_attr *attr,
					 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	val->value.i = priv->port_ingress_mode_override[port];

	return 0;
}

int
ar8327_sw_set_port_egress_mode_override(struct switch_dev *dev,
					const struct switch_attr *attr,
					struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	priv->port_egress_mode_override[port] = (val->value.i ? true : false);

	return 0;
}

int
ar8327_sw_get_port_egress_mode_override(struct switch_dev *dev,
					const struct switch_attr *attr,
					struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	val->value.i = priv->port_egress_mode_override[port];

	return 0;
}

int
ar8327_sw_get_egress_cir(struct switch_dev *dev,
			 const struct switch_attr *attr,
			 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	val->value.i = priv->port_qos_egress_cir[port];

	return 0;
}

int
ar8327_sw_get_egress_eir(struct switch_dev *dev,
			 const struct switch_attr *attr,
			 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	val->value.i = priv->port_qos_egress_eir[port];

	return 0;
}

int
ar8327_sw_set_egress_cir(struct switch_dev *dev,
			 const struct switch_attr *attr,
			 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	priv->port_qos_egress_cir[port] = val->value.i & 0x7FFF;

	return 0;
}

int
ar8327_sw_set_egress_eir(struct switch_dev *dev,
			 const struct switch_attr *attr,
			 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	priv->port_qos_egress_eir[port] = val->value.i & 0x7FFF;

	return 0;
}

int
ar8327_sw_get_port_igmp_snooping(struct switch_dev *dev,
				 const struct switch_attr *attr,
				 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	mutex_lock(&priv->reg_mutex);
	val->value.i = ar8327_get_port_igmp(priv, port);
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

int
ar8327_sw_set_port_igmp_snooping(struct switch_dev *dev,
				 const struct switch_attr *attr,
				 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;

	if (port >= dev->ports)
		return -EINVAL;

	mutex_lock(&priv->reg_mutex);
	ar8327_set_port_igmp(priv, port, val->value.i);
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

int
ar8327_sw_get_igmp_snooping(struct switch_dev *dev,
			    const struct switch_attr *attr,
			    struct switch_val *val)
{
	int port;

	for (port = 0; port < dev->ports; port++) {
		val->port_vlan = port;
		if (ar8327_sw_get_port_igmp_snooping(dev, attr, val) ||
		    !val->value.i)
			break;
	}

	return 0;
}

int
ar8327_sw_set_igmp_snooping(struct switch_dev *dev,
			    const struct switch_attr *attr,
			    struct switch_val *val)
{
	int port;

	for (port = 0; port < dev->ports; port++) {
		val->port_vlan = port;
		if (ar8327_sw_set_port_igmp_snooping(dev, attr, val))
			break;
	}

	return 0;
}

int
ar8327_sw_get_igmp_v3(struct switch_dev *dev,
		      const struct switch_attr *attr,
		      struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	u32 val_reg;

	mutex_lock(&priv->reg_mutex);
	val_reg = ar8xxx_read(priv, AR8327_REG_FRAME_ACK_CTRL1);
	val->value.i = ((val_reg & AR8327_FRAME_ACK_CTRL_IGMP_V3_EN) != 0);
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

int
ar8327_sw_set_igmp_v3(struct switch_dev *dev,
		      const struct switch_attr *attr,
		      struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);

	mutex_lock(&priv->reg_mutex);
	if (val->value.i)
		ar8xxx_reg_set(priv, AR8327_REG_FRAME_ACK_CTRL1,
			       AR8327_FRAME_ACK_CTRL_IGMP_V3_EN);
	else
		ar8xxx_reg_clear(priv, AR8327_REG_FRAME_ACK_CTRL1,
				 AR8327_FRAME_ACK_CTRL_IGMP_V3_EN);
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

int
ar8327_sw_set_vlan_detected(struct switch_dev *dev,
			    const struct switch_attr *attr,
			    struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);

	if (!priv->vlan_detected_ports) {
		pr_err("ar8327: vlan_detected_ports not set yet!\n");
		return -EINVAL;
	}

	if (val->value.i <= 0 || val->value.i >= 4096)
		return -EINVAL;

	mutex_lock(&priv->reg_mutex);
	ar8327_vtu_load_vlan(priv, val->value.i, priv->vlan_detected_ports);
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

int
ar8327_sw_set_vlan_detected_ports(struct switch_dev *dev,
				  const struct switch_attr *attr,
				  struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);

	if (val->value.i < 0 || val->value.i >= (1 << AR8327_NUM_PORTS))
		return -EINVAL;

	priv->vlan_detected_ports = val->value.i;
	return 0;
}

int
ar8327_sw_get_vlan_detected_ports(struct switch_dev *dev,
				  const struct switch_attr *attr,
				  struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	val->value.i = priv->vlan_detected_ports;
	return 0;
}

static int
ar8327_write_acl_rule(struct ar8xxx_priv *priv, u8 action, u8 rule_idx, u32* acl_regs)
{
	int reg_id;

	if (ar8216_wait_bit(priv, AR8327_REG_ACL_FUNC(0), AR8327_REG_FUNC0_ACL_BUSY, 0)) {
		printk("%s: waiting for ACL %d:%d timed out!\n", __func__, rule_idx, action);
		return -1;
	}

	acl_regs[0] |= AR8327_REG_FUNC0_ACL_BUSY;
	acl_regs[0] |= (action << AR8327_REG_FUNC0_ACL_RULE_SEL_S) & AR8327_REG_FUNC0_ACL_RULE_SEL;
	acl_regs[0] |= (rule_idx << AR8327_REG_FUNC0_ACL_FUNC_INDEX_S) & AR8327_REG_FUNC0_ACL_FUNC_INDEX;

	for (reg_id = AR8327_REG_ACL_CNT - 1; reg_id >= 0; reg_id--) {
		ar8xxx_write(priv, AR8327_REG_ACL_FUNC(reg_id), acl_regs[reg_id]);
#if AR8327_ACL_DEBUG
		printk("Written 0x%x to 0x%x\n", acl_regs[reg_id], AR8327_REG_ACL_FUNC(reg_id));
#endif
	}

	if (ar8216_wait_bit(priv, AR8327_REG_ACL_FUNC(0), AR8327_REG_FUNC0_ACL_BUSY, 0)) {
		printk("%s: waiting to set ACL %d:%d timed out!\n", __func__, rule_idx, action);
		return -1;
	}

	return 0;
}

#if AR8327_ACL_DEBUG
static int
ar8327_read_acl_rule(struct ar8xxx_priv *priv, u8 action, u8 rule_idx)
{
	u32 func_reg = 0;
	int reg_id;

	if (ar8216_wait_bit(priv, AR8327_REG_ACL_FUNC(0), AR8327_REG_FUNC0_ACL_BUSY, 0)) {
		printk("%s: waiting for ACL %d:%d timed out!\n", __func__, rule_idx, action);
		return -1;
	}

	func_reg |= AR8327_REG_FUNC0_ACL_BUSY;
	func_reg |= AR8327_REG_FUNC0_ACL_READ;
	func_reg |= (action << AR8327_REG_FUNC0_ACL_RULE_SEL_S) & AR8327_REG_FUNC0_ACL_RULE_SEL;
	func_reg |= (rule_idx << AR8327_REG_FUNC0_ACL_FUNC_INDEX_S) & AR8327_REG_FUNC0_ACL_FUNC_INDEX;

	ar8xxx_write(priv, AR8327_REG_ACL_FUNC(0), func_reg);

	if (ar8216_wait_bit(priv, AR8327_REG_ACL_FUNC(0), AR8327_REG_FUNC0_ACL_BUSY, 0)) {
		printk("%s: waiting to read ACL %d:%d timed out!\n", __func__, rule_idx, action);
		return -1;
	}

	printk("Reading ACL %d:%d:\n", action, rule_idx);
	for (reg_id = 0; reg_id < AR8327_REG_ACL_CNT; reg_id++)
		printk("Read 0x%x from 0x%x\n", ar8xxx_read(priv, AR8327_REG_ACL_FUNC(reg_id)), AR8327_REG_ACL_FUNC(reg_id));

	return 0;
}
#endif /* AR8327_ACL_DEBUG */

static int
ar8327_sw_set_802_1d_redirect(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	u32 acl_regs[6];
	
	/* MAC addresses in the range 01-80-C2-00-00-0* belong to "MAC Bridge Filtered MAC Group Addresses" in
	   IEEE standard 802.1D. MAC frames that have a destination MAC address within this range are not
	   relayed by MAC bridges conforming to IEEE 802.1D. */

	memset(acl_regs, 0, sizeof(acl_regs));
	if (val->value.i) {
		acl_regs[1] |= (ACL_802_1D_DST_MAC_3_0 << ACL_MAC_PATTERN_REG1_DEST_ADDR_3_0_S) & ACL_MAC_PATTERN_REG1_DEST_ADDR_3_0;
		acl_regs[2] |= (ACL_802_1D_DST_MAC_5_4 << ACL_MAC_PATTERN_REG2_DEST_ADDR_5_4_S) & ACL_MAC_PATTERN_REG2_DEST_ADDR_5_4;
		acl_regs[5] |= (AR8327_PORTS_ALL_PHYSICAL << ACL_MAC_PATTERN_REG5_SOURCE_PORT_S) & ACL_MAC_PATTERN_REG5_SOURCE_PORT;
	}
	if (ar8327_write_acl_rule(priv, AR8327_ACL_SEL_RULE, ACL_802_1D_RULE_ID, acl_regs))
		return -ETIMEDOUT;
	
	memset(acl_regs, 0, sizeof(acl_regs));
	if (val->value.i) {
		acl_regs[1] |= (ACL_802_1D_MAC_MASK_3_0 << ACL_MAC_MASK_REG1_DEST_ADDR_3_0_S) & ACL_MAC_MASK_REG1_DEST_ADDR_3_0;
		acl_regs[2] |= (ACL_802_1D_MAC_MASK_5_4 << ACL_MAC_MASK_REG2_DEST_ADDR_5_4_S) & ACL_MAC_MASK_REG2_DEST_ADDR_5_4;
		acl_regs[5] |= (ACL_MAC_RULE_TYPE << ACL_MAC_MASK_REG5_RULE_TYPE_S) & ACL_MAC_MASK_REG5_RULE_TYPE;
		acl_regs[5] |= ACL_MAC_MASK_REG5_VID_MASK;
		acl_regs[5] |= (ACL_MAC_MASK_REG5_RULE_VALID_START_END << ACL_MAC_MASK_REG5_RULE_VALID_S) & ACL_MAC_MASK_REG5_RULE_VALID;
	}
	if (ar8327_write_acl_rule(priv, AR8327_ACL_SEL_MASK, ACL_802_1D_RULE_ID, acl_regs))
		return -ETIMEDOUT;

	memset(acl_regs, 0, sizeof(acl_regs));
	if (val->value.i) {
		acl_regs[2] |= (AR8327_PORT_CPU << ACL_ACTION_REG2_DES_PORT_0_2_S) & ACL_ACTION_REG2_DES_PORT_0_2;
		acl_regs[3] |= ACL_ACTION_REG3_DES_PORT_OVER_EN;
	}
	if (ar8327_write_acl_rule(priv, AR8327_ACL_SEL_ACTION, ACL_802_1D_RULE_ID, acl_regs))
		return -ETIMEDOUT;

	priv->acl_802_1d_redirect = !!val->value.i;
	
	return 0;
}
	
static int
ar8327_sw_get_802_1d_redirect(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	val->value.i = priv->acl_802_1d_redirect;

#if AR8327_ACL_DEBUG
	ar8327_read_acl_rule(priv, AR8327_ACL_SEL_RULE, ACL_802_1D_RULE_ID);
	ar8327_read_acl_rule(priv, AR8327_ACL_SEL_MASK, ACL_802_1D_RULE_ID);
	ar8327_read_acl_rule(priv, AR8327_ACL_SEL_ACTION, ACL_802_1D_RULE_ID);
#endif
	return 0;	
}

int
ar8327_sw_get_port_table(struct switch_dev *dev,
			 const struct switch_attr *attr,
			 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;
	u32 val_reg;

	if (port >= dev->ports)
		return -EINVAL;

	mutex_lock(&priv->reg_mutex);
	val_reg = ar8xxx_read(priv, AR8327_REG_PORT_LOOKUP(port));
	val->value.i = val_reg & AR8327_PORT_LOOKUP_MEMBER;
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

int
ar8327_sw_set_port_table(struct switch_dev *dev,
			 const struct switch_attr *attr,
			 struct switch_val *val)
{
	struct ar8xxx_priv *priv = swdev_to_ar8xxx(dev);
	int port = val->port_vlan;
	u32 port_vid_mem = val->value.i & 0x7F;

	if (port >= dev->ports)
		return -EINVAL;

	port_vid_mem &= ~(1 << port); // shouldn't forward to this port

	mutex_lock(&priv->reg_mutex);
	ar8xxx_rmw(priv, AR8327_REG_PORT_LOOKUP(port), AR8327_PORT_LOOKUP_MEMBER, port_vid_mem);
	mutex_unlock(&priv->reg_mutex);

	return 0;
}

static const struct switch_attr ar8327_sw_attr_globals[] = {
	{
		.type = SWITCH_TYPE_INT,
		.name = "enable_vlan",
		.description = "Enable VLAN mode",
		.set = ar8xxx_sw_set_vlan,
		.get = ar8xxx_sw_get_vlan,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_NOVAL,
		.name = "reset_mibs",
		.description = "Reset all MIB counters",
		.set = ar8xxx_sw_set_reset_mibs,
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "enable_mirror_rx",
		.description = "Enable mirroring of RX packets",
		.set = ar8xxx_sw_set_mirror_rx_enable,
		.get = ar8xxx_sw_get_mirror_rx_enable,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "enable_mirror_tx",
		.description = "Enable mirroring of TX packets",
		.set = ar8xxx_sw_set_mirror_tx_enable,
		.get = ar8xxx_sw_get_mirror_tx_enable,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "mirror_monitor_port",
		.description = "Mirror monitor port",
		.set = ar8xxx_sw_set_mirror_monitor_port,
		.get = ar8xxx_sw_get_mirror_monitor_port,
		.max = AR8327_NUM_PORTS - 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "mirror_source_port",
		.description = "Mirror source port",
		.set = ar8xxx_sw_set_mirror_source_port,
		.get = ar8xxx_sw_get_mirror_source_port,
		.max = AR8327_NUM_PORTS - 1
 	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "arl_age_time",
		.description = "ARL age time (secs)",
		.set = ar8xxx_sw_set_arl_age_time,
		.get = ar8xxx_sw_get_arl_age_time,
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "arl_cache_age_time",
		.description = "ARL cache age time (secs)",
		.set = ar8xxx_sw_set_arl_cache_age_time,
		.get = ar8xxx_sw_get_arl_cache_age_time,
	},
	{
		.type = SWITCH_TYPE_STRING,
		.name = "arl_table",
		.description = "Get ARL table",
		.set = NULL,
		.get = ar8xxx_sw_get_arl_table,
	},
	{
		.type = SWITCH_TYPE_NOVAL,
		.name = "flush_arl_table",
		.description = "Flush ARL table",
		.set = ar8xxx_sw_set_flush_arl_table,
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "igmp_snooping",
		.description = "Enable IGMP Snooping",
		.set = ar8327_sw_set_igmp_snooping,
		.get = ar8327_sw_get_igmp_snooping,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "igmp_v3",
		.description = "Enable IGMPv3 support",
		.set = ar8327_sw_set_igmp_v3,
		.get = ar8327_sw_get_igmp_v3,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "vlan_detected",
		.description = "Program VTU with new VLAN",
		.set = ar8327_sw_set_vlan_detected,
		.max = 4096
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "vlan_detected_ports",
		.description = "Bitmask of ports to program with newly detected VLANs",
		.set = ar8327_sw_set_vlan_detected_ports,
		.get = ar8327_sw_get_vlan_detected_ports,
		.max = (1 << AR8327_NUM_PORTS) - 1,
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "802_1d_redirect",
		.description = "Redirect frames with Bridge Filtered MAC Group Addresses from physical ports to CPU",
		.set = ar8327_sw_set_802_1d_redirect,
		.get = ar8327_sw_get_802_1d_redirect,
		.max = 1,
	},
};

static const struct switch_attr ar8327_sw_attr_port[] = {
	{
		.type = SWITCH_TYPE_NOVAL,
		.name = "reset_mib",
		.description = "Reset single port MIB counters",
		.set = ar8xxx_sw_set_port_reset_mib,
	},
	{
		.type = SWITCH_TYPE_STRING,
		.name = "mib",
		.description = "Get port's MIB counters",
		.set = NULL,
		.get = ar8xxx_sw_get_port_mib,
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "enable_eee",
		.description = "Enable EEE PHY sleep mode",
		.set = ar8327_sw_set_eee,
		.get = ar8327_sw_get_eee,
		.max = 1,
	},
	{
		.type = SWITCH_TYPE_NOVAL,
		.name = "flush_arl_table",
		.description = "Flush port's ARL table entries",
		.set = ar8xxx_sw_set_flush_port_arl_table,
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "disable",
		.description = "Port state (1:disabled)",
		.max = 1,
		.get = ar8xxx_sw_get_port_disable,
		.set = ar8xxx_sw_set_port_disable,
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "igmp_snooping",
		.description = "Enable port's IGMP Snooping",
		.set = ar8327_sw_set_port_igmp_snooping,
		.get = ar8327_sw_get_port_igmp_snooping,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "ingress_mode_val",
		.description = "Get/Set port's ingress override mode value",
		.set = ar8327_sw_set_port_ingress_mode,
		.get = ar8327_sw_get_port_ingress_mode,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "egress_mode_val",
		.description = "Get/Set port's egress override mode value",
		.set = ar8327_sw_set_port_egress_mode,
		.get = ar8327_sw_get_port_egress_mode,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "ingress_mode_override",
		.description = "Enable port's ingress override mode",
		.set = ar8327_sw_set_port_ingress_mode_override,
		.get = ar8327_sw_get_port_ingress_mode_override,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "egress_mode_override",
		.description = "Enable port's egress override mode",
		.set = ar8327_sw_set_port_egress_mode_override,
		.get = ar8327_sw_get_port_egress_mode_override,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "qos_egress_cir",
		.description = "Limit port's egress Committed Information Rate in multiples of 32Kbps, max = 32767",
		.set = ar8327_sw_set_egress_cir,
		.get = ar8327_sw_get_egress_cir,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "qos_egress_eir",
		.description = "Limit port's egress Excess Information Rate in multiples of 32Kbps, max = 32767",
		.set = ar8327_sw_set_egress_eir,
		.get = ar8327_sw_get_egress_eir,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "stp_state",
		.description = "Spanning Tree States: 0 = disable, 1 = block, 2 = listen, 3 = learn, 4 = forward",
		.set = ar8xxx_sw_set_port_stp_state,
		.get = ar8xxx_sw_get_port_stp_state,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "port_table",
		.description = "Override port table bitmap",
		.set = ar8327_sw_set_port_table,
		.get = ar8327_sw_get_port_table,
		.max = 1
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "fdx_advertise",
		.description = "Advertise full duplex",
		.max = 1,
		.get = ar8xxx_sw_get_fdx_advertise,
		.set = ar8xxx_sw_set_fdx_advertise,
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "hdx_advertise",
		.description = "Advertise half duplex",
		.max = 1,
		.get = ar8xxx_sw_get_hdx_advertise,
		.set = ar8xxx_sw_set_hdx_advertise,
	},
	{
		.type = SWITCH_TYPE_INT,
		.name = "speed_advertise",
		.description = "Advertised max speed (valid values: 10, 100, 1000)",
		.get = ar8xxx_sw_get_speed_advertise,
		.set = ar8xxx_sw_set_speed_advertise,
	},
};

static const struct switch_dev_ops ar8327_sw_ops = {
	.attr_global = {
		.attr = ar8327_sw_attr_globals,
		.n_attr = ARRAY_SIZE(ar8327_sw_attr_globals),
	},
	.attr_port = {
		.attr = ar8327_sw_attr_port,
		.n_attr = ARRAY_SIZE(ar8327_sw_attr_port),
	},
	.attr_vlan = {
		.attr = ar8xxx_sw_attr_vlan,
		.n_attr = ARRAY_SIZE(ar8xxx_sw_attr_vlan),
	},
	.get_port_pvid = ar8xxx_sw_get_pvid,
	.set_port_pvid = ar8xxx_sw_set_pvid,
	.get_vlan_ports = ar8327_sw_get_ports,
	.set_vlan_ports = ar8327_sw_set_ports,
	.apply_config = ar8327_sw_hw_apply,
	.reset_switch = ar8xxx_sw_reset_switch,
	.get_port_link = ar8xxx_sw_get_port_link,
};

const struct ar8xxx_chip ar8327_chip = {
	.caps = AR8XXX_CAP_GIGE | AR8XXX_CAP_MIB_COUNTERS,
	.config_at_probe = true,
	.mii_lo_first = true,

	.name = "Atheros AR8327",
	.ports = AR8327_NUM_PORTS,
	.vlans = AR8X16_MAX_VLANS,
	.swops = &ar8327_sw_ops,

	.reg_port_stats_start = 0x1000,
	.reg_port_stats_length = 0x100,
	.reg_arl_ctrl = AR8327_REG_ARL_CTRL,

	.hw_init = ar8327_hw_init,
	.cleanup = ar8327_cleanup,
	.init_globals = ar8327_init_globals,
	.init_port = ar8327_init_port,
	.setup_port = ar8327_setup_port,
	.read_port_status = ar8327_read_port_status,
	.read_port_eee_status = ar8327_read_port_eee_status,
	.atu_flush = ar8327_atu_flush,
	.atu_flush_port = ar8327_atu_flush_port,
	.vtu_flush = ar8327_vtu_flush,
	.vtu_load_vlan = ar8327_vtu_load_vlan,
	.set_mirror_regs = ar8327_set_mirror_regs,
	.get_arl_entry = ar8327_get_arl_entry,
	.sw_hw_apply = ar8327_sw_hw_apply,

	.num_mibs = ARRAY_SIZE(ar8236_mibs),
	.mib_decs = ar8236_mibs,
	.mib_func = AR8327_REG_MIB_FUNC
};

const struct ar8xxx_chip ar8337_chip = {
	.caps = AR8XXX_CAP_GIGE | AR8XXX_CAP_MIB_COUNTERS,
	.config_at_probe = true,
	.mii_lo_first = true,

	.name = "Atheros AR8337",
	.ports = AR8327_NUM_PORTS,
	.vlans = AR8X16_MAX_VLANS,
	.swops = &ar8327_sw_ops,

	.reg_port_stats_start = 0x1000,
	.reg_port_stats_length = 0x100,
	.reg_arl_ctrl = AR8327_REG_ARL_CTRL,

	.hw_init = ar8327_hw_init,
	.cleanup = ar8327_cleanup,
	.init_globals = ar8327_init_globals,
	.init_port = ar8327_init_port,
	.setup_port = ar8327_setup_port,
	.read_port_status = ar8327_read_port_status,
	.read_port_eee_status = ar8327_read_port_eee_status,
	.atu_flush = ar8327_atu_flush,
	.atu_flush_port = ar8327_atu_flush_port,
	.vtu_flush = ar8327_vtu_flush,
	.vtu_load_vlan = ar8327_vtu_load_vlan,
	.phy_fixup = ar8327_phy_fixup,
	.set_mirror_regs = ar8327_set_mirror_regs,
	.get_arl_entry = ar8327_get_arl_entry,
	.sw_hw_apply = ar8327_sw_hw_apply,

	.num_mibs = ARRAY_SIZE(ar8236_mibs),
	.mib_decs = ar8236_mibs,
	.mib_func = AR8327_REG_MIB_FUNC
};
