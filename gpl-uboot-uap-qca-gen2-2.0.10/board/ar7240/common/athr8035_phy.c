/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
 */

#include <config.h>
#include <linux/types.h>
#include <common.h>
#include <miiphy.h>
#include "phy.h"
#include <asm/addrspace.h>
#include "ar7240_soc.h"
#include "athr8035_phy.h"

#define MODULE_NAME "ATHR8035"
//#define CONFIGURABLE_GIGE
//#define PHY_WAIT_AUTONEG 

#define sysMsDelay(_x) udelay((_x) * 1000)
#define DRV_PRINT(fmt...) do { if (eth_debug > 0) { printf(fmt); } } while (0)

static int eth_debug = 1;

typedef struct {
  int              is_enet_port;
  int              mac_unit;
  unsigned int     phy_addr;
  unsigned int     id1;
  unsigned int     id2;
} athr8035_phy_t;

static athr8035_phy_t phy_info[] = {
    {is_enet_port: 1,
     mac_unit    : 0,
     phy_addr    : 0x04,
     id1         : ATHR_PHY_ID1_EXPECTED,
     id2         : ATHR_AR8035_PHY_ID2_EXPECTED },
};

static athr8035_phy_t *
athr8035_phy_find(int unit)
{
    unsigned int i;
    athr8035_phy_t *phy;
    u32 id1, id2;

	for(i = 0; i < sizeof(phy_info)/sizeof(athr8035_phy_t); i++) {
		phy = &phy_info[i];
		if (phy->is_enet_port && (phy->mac_unit == unit)) {
			id1 = phy_reg_read(unit, phy->phy_addr, ATHR_PHY_ID1);
                        id2 = phy_reg_read(unit, phy->phy_addr, ATHR_PHY_ID2);
			if ((id1 == phy->id1) && (id2 == phy->id2)) {
				return phy;
			}
		}
	}
    return NULL;
}

static int getenv_int(const char* var) {
	const char* val = getenv(var);
	if (!val)
		return 0;
	return simple_strtol(val, NULL, 10);
}

BOOL
athr8035_phy_probe(int unit)
{
	static int found = 0;
	// If already detected, skip search.  If not yet found, always check again
	if(!found) {
		found = !!athr8035_phy_find(unit);
		if(found)
			DRV_PRINT( "AR8035 Detected\n");
	}
	return found;
}

BOOL
athr8035_phy_setup(int unit)
{
    athr8035_phy_t *phy = athr8035_phy_find(unit);
    uint16_t  phyHwStatus;
    uint16_t  timeout;

    eth_debug = getenv_int("ethdebug");

    DRV_PRINT("%s: enter athr8035_phy_setup.  MAC unit %d!\n", MODULE_NAME, unit);

    if (!phy) {
        DRV_PRINT("%s: \nNo phy found for unit %d\n", MODULE_NAME, unit);
        return 0;
    }

    /*
     * After the phy is reset, it takes a little while before
     * it can respond properly.
     */
    phy_reg_write(unit, phy->phy_addr, ATHR_AUTONEG_ADVERT,
                  ATHR_ADVERTISE_ALL);

    phy_reg_write(unit, phy->phy_addr, ATHR_1000BASET_CONTROL,
                  ATHR_ADVERTISE_1000FULL);

    {
	    u_int32_t phy_rx_clock_delay = 1;
	    u_int32_t phy_tx_clock_delay = 1;
	    u_int32_t phy_smart_speed = 1;
	    u_int32_t phy_smart_speed_retry_limit_val = 3;
	    u_int32_t phy_tx_clock_delay_val = 0x2;
	    u_int32_t phy_bypass_smart_speed_timer_val = 0;
#ifdef CONFIGURABLE_GIGE
	    const char* phy_rx_clock_delay_value = getenv("phy_rx_clock_delay");
	    const char* phy_smart_speed_value = getenv("phy_smart_speed");
	    const char* phy_smart_speed_retry_val = getenv("phy_smart_speed_retry_val");
	    const char* phy_tx_clock_delay_value = getenv("phy_tx_clock_delay");
	    const char* phy_tx_clock_delay_val_value = getenv("phy_tx_clock_delay_val");
	    const char* phy_bypass_smart_speed_timer = getenv("phy_bypass_smart_speed_timer");
	    DRV_PRINT( "-------------------------------------------------------\n");
	    DRV_PRINT( "                 PHY - SmartSpeed\n");
	    DRV_PRINT( "-------------------------------------------------------\n");
	    if (!phy_smart_speed_value) {
		    DRV_PRINT( "  STATUS: No override 'phy_smart_speed' environment variable found.\n");
	    } else {
		    phy_smart_speed = simple_strtoul(phy_smart_speed_value, NULL, 10);
		    DRV_PRINT( "  STATUS: Override 'phy_smart_speed' environment variable: %s --> %d\n", phy_smart_speed_value, phy_smart_speed);
	    }
	    if (!phy_bypass_smart_speed_timer) {
		    DRV_PRINT( "  STATUS: No override 'phy_bypass_smart_speed_timer' environment variable found.\n");
	    } else {
		    phy_bypass_smart_speed_timer_val = simple_strtoul(phy_bypass_smart_speed_timer, NULL, 10);
		    DRV_PRINT( "  STATUS: Override 'phy_bypass_smart_speed_timer' environment variable: %s --> %d\n", phy_bypass_smart_speed_timer, phy_bypass_smart_speed_timer_val);
	    }
	    if (!phy_smart_speed_retry_val) {
		    DRV_PRINT( "  STATUS: No override 'phy_smart_speed_retry_val' environment variable found.\n");
	    } else {
		    phy_smart_speed_retry_limit_val = simple_strtoul(phy_smart_speed_retry_val, NULL, 10);
		    DRV_PRINT( "  STATUS: Override 'phy_smart_speed_retry_val' environment variable: %s --> %d\n", phy_smart_speed_retry_val, phy_smart_speed_retry_limit_val);
	    }
	    DRV_PRINT( "  ENABLED: %s\n", phy_smart_speed ? "YES" : "NO");
	    DRV_PRINT( "-------------------------------------------------------\n");
	    DRV_PRINT( "        PHY - SEL_CLK125M_DSP (RX Clock Delay)\n");
	    DRV_PRINT( "-------------------------------------------------------\n");
	    if (!phy_rx_clock_delay_value) {
		    DRV_PRINT( "  STATUS: No override 'phy_rx_clock_delay' environment variable found.\n");
	    } else {
		    phy_rx_clock_delay = simple_strtoul(phy_rx_clock_delay_value, NULL, 10);
		    DRV_PRINT( "  STATUS: Override 'phy_rx_clock_delay' environment variable: %s --> %d\n", phy_rx_clock_delay_value, phy_rx_clock_delay);
	    }
	    DRV_PRINT( "  ENABLED: %s\n", phy_rx_clock_delay ? "YES" : "NO");
	    DRV_PRINT( "-------------------------------------------------------\n");
	    DRV_PRINT( "        PHY - RGMII_TX_CLK_DLY (TX Clock Delay)\n");
	    DRV_PRINT( "-------------------------------------------------------\n");
	    if (!phy_tx_clock_delay_value) {
		    DRV_PRINT( "  STATUS: No override 'phy_tx_clock_delay' environment variable found.\n");
	    } else {
		   phy_tx_clock_delay = simple_strtoul(phy_tx_clock_delay_value, NULL, 10);
		   DRV_PRINT( "  STATUS: Override 'phy_tx_clock_delay' environment variable: %s --> %d\n", phy_tx_clock_delay_value, phy_tx_clock_delay);
	    }
	    DRV_PRINT( "  ENABLED: %s\n", phy_tx_clock_delay ? "YES" : "NO");
	    DRV_PRINT( "-------------------------------------------------------\n");
	    DRV_PRINT( "        PHY - GTX_DLY_VAL (TX Clock Delay Value)\n");
	    DRV_PRINT( "-------------------------------------------------------\n");
	    if (!phy_tx_clock_delay_val_value) {
		    DRV_PRINT( "  STATUS: No override 'phy_tx_clock_delay_val' environment variable found.\n");
	    } else {
		    phy_tx_clock_delay_val = simple_strtoul(phy_tx_clock_delay_val_value, NULL, 10);
		    DRV_PRINT( "  STATUS: Override 'phy_tx_clock_delay_val' environment variable: %s --> %d\n", phy_tx_clock_delay_val_value, phy_tx_clock_delay_val);
	    }
	    if(phy_tx_clock_delay_val > 0x3) {
		    DRV_PRINT( "  WARNING: phy_tx_clock_delay_val range is 0x0-0x3.  Reduced input, 0x%x, to 0x%x.\n", phy_tx_clock_delay_val, 0x3);
		    phy_tx_clock_delay_val = 0x3;
	    }
	    {
		    char* len = "0.25ns";
		    switch(phy_tx_clock_delay_val) {
		    case 0:
			    len ="0.25ns";
			    break;
		    case 1:
			    len = "1.3ns";
			    break;
		    case 2:
			    len = "2.4ns";
			    break;
		    case 3:
			    len="3.4ns";
			    break;
		    }
		    DRV_PRINT( "  VALUE: 0x%x (%s)\n", phy_tx_clock_delay_val, len);
	    }
	    DRV_PRINT( "-------------------------------------------------------\n");
#endif // #ifdef CONFIGURABLE_GIGE
	    /* necessary PHY FIXUP: delay rx_clk */
	    phy_reg_write(unit, phy->phy_addr, 0x1D, 0x0);
	    phy_reg_write(unit, phy->phy_addr, 0x1E, (phy_reg_read(unit, phy->phy_addr, 0x1E) & ~0x8000) | (phy_rx_clock_delay ? 0x8000 : 0x0));
	    /* necessary PHY FIXUP: phy_tx_clock_delay */
	    phy_reg_write(unit, phy->phy_addr, 0x1D, 0x5);
	    phy_reg_write(unit, phy->phy_addr, 0x1E, (phy_reg_read(unit, phy->phy_addr, 0x1E) & ~0x100) | (phy_tx_clock_delay ? 0x0100 : 0x0));
	    /* Write delay gtx_dly_val */
	    if(phy_tx_clock_delay_val != 2) {
		    phy_reg_write(unit, phy->phy_addr, 0x1D, 0xB);
		    phy_reg_write(unit, phy->phy_addr, 0x1E, (phy_reg_read(unit, phy->phy_addr, 0x1E) & ~(0x3 << 5)) | ((phy_tx_clock_delay_val & 0x3) << 5) );
	    }
	    {
		    u_int32_t x = 0;
		    x |= (phy_smart_speed ? (1<<5) : 0);
		    x |= ((phy_smart_speed_retry_limit_val & 0x7) << 2);
		    x |= !!(phy_bypass_smart_speed_timer_val);
		    phy_reg_write(unit, phy->phy_addr, 0x14, x);
	    }
	    // Add a default XMII config value to GigE settings
	    if (is_ar7242() && (unit == 0)) {
		ar7240_reg_wr(AR7242_ETH_XMII_CONFIG, athr8035_phy_get_xmii_config(unit,_1000BASET));
		udelay(100*1000);
	    }
	    /* Reset PHYs*/
	    phy_reg_write(unit, phy->phy_addr, ATHR_PHY_CONTROL, 0 | ATHR_CTRL_SOFTWARE_RESET | ATHR_CTRL_AUTONEGOTIATION_ENABLE );
	    /* Write smartspeed on/off */
	    {
		    u_int32_t x = 0;
		    x |= (phy_smart_speed ? (1<<5) : 0);
		    x |= ((phy_smart_speed_retry_limit_val & 0x7) << 2);
		    x |= !!(phy_bypass_smart_speed_timer_val);
		    phy_reg_write(unit, phy->phy_addr, 0x14, x);
	    }
	    sysMsDelay(100);
	    // Add a default XMII config value to GigE settings
	    if (is_ar7242() && (unit == 0)) {
		ar7240_reg_wr(AR7242_ETH_XMII_CONFIG, athr8035_phy_get_xmii_config(unit,_1000BASET));
		udelay(100*1000);
	    }
    }

    /*
     * Wait up to 3 seconds for ALL associated PHYs to finish
     * autonegotiation.  The only way we get out of here sooner is
     * if ALL PHYs are connected AND finish autonegotiation.
     */
    for (timeout = 300; timeout; sysMsDelay(10), timeout--) {
        phyHwStatus = phy_reg_read(unit, phy->phy_addr, ATHR_PHY_CONTROL);

	if (!ATHR_RESET_DONE(phyHwStatus))
		continue;
#ifndef PHY_WAIT_AUTONEG
	else {
		sysMsDelay(100);
		break;
	}
#else
        phyHwStatus = phy_reg_read(unit, phy->phy_addr, ATHR_PHY_STATUS);
        if (ATHR_AUTONEG_DONE(phyHwStatus)) {
            break;
        }
#endif
    }
    if (ATHR_AUTONEG_DONE(phyHwStatus)) {
	    DRV_PRINT("%s: Port %d, Negotiation Success\n", MODULE_NAME, unit);
    } else {
#ifdef PHY_WAIT_AUTONEG
	    DRV_PRINT("%s: Port %d, Negotiation Timeout\n", MODULE_NAME, unit);
#endif
    }

    athr8035_phy_speed(unit);

    return 1;
}

int
athr8035_phy_is_up(int unit)
{
    int status;
    athr8035_phy_t *phy = athr8035_phy_find(unit);

    DRV_PRINT("%s: enter athr8035_phy_is_up!\n", MODULE_NAME);
    if (!phy) {
        DRV_PRINT("%s: phy unit %d not found !\n", MODULE_NAME, unit);
       	return 0;
    }

    status = phy_reg_read(unit, phy->phy_addr, ATHR_PHY_SPEC_STATUS);

    if (status & ATHR_STATUS_LINK_PASS) {
	DRV_PRINT("%s: athr8035 Link up! \n", MODULE_NAME);
        return 1;
    }

    DRV_PRINT("%s: athr8035 Link down! \n", MODULE_NAME);
    return 0;
}

int
athr8035_phy_is_fdx(int unit)
{
    int status;
    athr8035_phy_t *phy = athr8035_phy_find(unit);
    int ii = 200;

    DRV_PRINT("%s: enter athr8035_phy_is_fdx!\n", MODULE_NAME);

    if (!phy) {
        DRV_PRINT("%s: phy unit %d not found !\n", MODULE_NAME, unit);
       	return 0;
    }

    do {
	    status = phy_reg_read(unit, phy->phy_addr, ATHR_PHY_SPEC_STATUS);
	    sysMsDelay(10);
    } while((!(status & ATHR_STATUS_RESOLVED)) && --ii);
    status = !(!(status & ATHER_STATUS_FULL_DEPLEX));

    return (status);
}

void athr8035_phy_reg_dump(int mac_unit, int phy_unit);

void ar7240_sys_frequency(u32 *cpu_freq, u32 *ddr_freq, u32 *ahb_freq);
u32 athr8035_phy_get_xmii_config(int macUnit, u32 speed) {
	switch (speed)
	{
	case _1000BASET:
		{
			
			u_int32_t xmii_config = 0x02000000;
#ifdef CONFIGURABLE_GIGE
			const char* eth_xmii_value = getenv("eth_xmii");
			DRV_PRINT( "-------------------------------------------------------\n");
			DRV_PRINT( "                  ETH_XMII_CONFIG\n");
			DRV_PRINT( "-------------------------------------------------------\n");
			if (!eth_xmii_value) {
				DRV_PRINT( "  STATUS: No override 'eth_xmii' environment variable found.\n");
			} else {
				xmii_config = simple_strtoul(eth_xmii_value, NULL, 16);
				DRV_PRINT( "  STATUS: Override 'eth_xmii' environment variable: %s --> 0x%08x\n", eth_xmii_value, xmii_config);
			}
			if(0 == (xmii_config & (1 << 25))) {
				DRV_PRINT( "  WARNING: You forgot bit 25 - GIGE\n");
				xmii_config |= (1 << 25);
			}
			DRV_PRINT( "  VALUE....................0x%08x\n", xmii_config);
			DRV_PRINT( "  TX INVERT................%s\n", (xmii_config & (1 << 31)) ? "ON" : "OFF");
			DRV_PRINT( "  GIGE_QUAD................%s\n", (xmii_config & (1 << 30)) ? "ON" : "OFF");
			DRV_PRINT( "  RX_DELAY_BUFFERS.........b%d%d (%d)\n", (xmii_config >> 29 ) & 1, (xmii_config >> 28 ) & 1, (xmii_config >> 28 ) & 0x3);
			DRV_PRINT( "  TX_DELAY_LINE_SETTING....b%d%d (%d)\n", (xmii_config >> 27 ) & 1, (xmii_config >> 26 ) & 1, (xmii_config >> 26 ) & 0x3);
			DRV_PRINT( "  OFFSET PHASE.............%s\n", (xmii_config & (1 << 24 ) ) ? "180 degrees" : "0 degrees");
			DRV_PRINT( "  OFFSET COUNT.............0x%02x (%d)\n", (xmii_config >> 16 ) & 0xFF, (xmii_config >> 16 ) & 0xFF);
			DRV_PRINT( "  PHASE1_COUNT.............0x%02x (%d)\n", (xmii_config >> 8 ) & 0xFF,  (xmii_config >> 8 ) & 0xFF);
			DRV_PRINT( "  PHASE0_COUNT.............0x%02x (%d)\n", (xmii_config >> 8 ) & 0xFF,  (xmii_config >> 8 ) & 0xFF);
			DRV_PRINT( "-------------------------------------------------------\n");
#endif // #ifdef CONFIGURABLE_GIGE
			return xmii_config;
		}

	case _100BASET:
		{
		u_int32_t xmii_config = 0x0101;
#ifdef CONFIGURABLE_GIGE
			const char* eth_xmii_value = getenv("eth_xmii_100");
			DRV_PRINT( "-------------------------------------------------------\n");
			DRV_PRINT( "                  ETH_XMII_CONFIG\n");
			DRV_PRINT( "-------------------------------------------------------\n");
			if (!eth_xmii_value) {
				DRV_PRINT( "  STATUS: No override 'eth_xmii_100' environment variable found.\n");
			} else {
				xmii_config = simple_strtoul(eth_xmii_value, NULL, 16);
				DRV_PRINT( "  STATUS: Override 'eth_xmii_100' environment variable: %s --> 0x%08x\n", eth_xmii_value, xmii_config);
			}
			if(0 != (xmii_config & (1 << 25))) {
				DRV_PRINT( "  WARNING: You set bit 25 - GIGE in 100 Mbit mode\n");
				xmii_config &= ~(1 << 25);
			}
			DRV_PRINT( "  VALUE....................0x%08x\n", xmii_config);
			DRV_PRINT( "  TX INVERT................%s\n", (xmii_config & (1 << 31)) ? "ON" : "OFF");
			DRV_PRINT( "  GIGE_QUAD................%s\n", (xmii_config & (1 << 30)) ? "ON" : "OFF");
			DRV_PRINT( "  RX_DELAY_BUFFERS.........b%d%d (%d)\n", (xmii_config >> 29 ) & 1, (xmii_config >> 28 ) & 1, (xmii_config >> 28 ) & 0x3);
			DRV_PRINT( "  TX_DELAY_LINE_SETTING....b%d%d (%d)\n", (xmii_config >> 27 ) & 1, (xmii_config >> 26 ) & 1, (xmii_config >> 26 ) & 0x3);
			DRV_PRINT( "  OFFSET PHASE.............%s\n", (xmii_config & (1 << 24 ) ) ? "180 degrees" : "0 degrees");
			DRV_PRINT( "  OFFSET COUNT.............0x%02x (%d)\n", (xmii_config >> 16 ) & 0xFF, (xmii_config >> 16 ) & 0xFF);
			DRV_PRINT( "  PHASE1_COUNT.............0x%02x (%d)\n", (xmii_config >> 8 ) & 0xFF,  (xmii_config >> 8 ) & 0xFF);
			DRV_PRINT( "  PHASE0_COUNT.............0x%02x (%d)\n", (xmii_config >> 8 ) & 0xFF,  (xmii_config >> 8 ) & 0xFF);
			DRV_PRINT( "-------------------------------------------------------\n");
#endif // #ifdef CONFIGURABLE_GIGE
			return xmii_config;
		}
	default:
	case _10BASET:
		{
		u_int32_t xmii_config = 0x0101;
		u32 ahb = 0;
		ar7240_sys_frequency(0, 0, &ahb);
		if (ahb == 195000000) {
			xmii_config = 0x1313; // NB: Because we have clock at 195
		} else if (ahb == 200000000) {
			xmii_config = 0x1616;
		} else {
			printf( "WARNING: Unsupported AHB frequency, %d.  10baseT clock may be wrong.\n", ahb);
			xmii_config = 0x1616;
		}
#ifdef CONFIGURABLE_GIGE
		const char* eth_xmii_value = getenv("eth_xmii_10");
		DRV_PRINT( "-------------------------------------------------------\n");
		DRV_PRINT( "                  ETH_XMII_CONFIG\n");
		DRV_PRINT( "-------------------------------------------------------\n");
		if (!eth_xmii_value) {
			DRV_PRINT( "  STATUS: No override 'eth_xmii_10' environment variable found.\n");
		} else {
			xmii_config = simple_strtoul(eth_xmii_value, NULL, 16);
			DRV_PRINT( "  STATUS: Override 'eth_xmii_10' environment variable: %s --> 0x%08x\n", eth_xmii_value, xmii_config);
		}
		if(0 != (xmii_config & (1 << 25))) {
			DRV_PRINT( "  WARNING: You set bit 25 - GIGE in 100 Mbit mode\n");
			xmii_config &= ~(1 << 25);
		}
			DRV_PRINT( "  VALUE....................0x%08x\n", xmii_config);
			DRV_PRINT( "  TX INVERT................%s\n", (xmii_config & (1 << 31)) ? "ON" : "OFF");
			DRV_PRINT( "  GIGE_QUAD................%s\n", (xmii_config & (1 << 30)) ? "ON" : "OFF");
			DRV_PRINT( "  RX_DELAY_BUFFERS.........b%d%d (%d)\n", (xmii_config >> 29 ) & 1, (xmii_config >> 28 ) & 1, (xmii_config >> 28 ) & 0x3);
			DRV_PRINT( "  TX_DELAY_LINE_SETTING....b%d%d (%d)\n", (xmii_config >> 27 ) & 1, (xmii_config >> 26 ) & 1, (xmii_config >> 26 ) & 0x3);
			DRV_PRINT( "  OFFSET PHASE.............%s\n", (xmii_config & (1 << 24 ) ) ? "180 degrees" : "0 degrees");
			DRV_PRINT( "  OFFSET COUNT.............0x%02x (%d)\n", (xmii_config >> 16 ) & 0xFF, (xmii_config >> 16 ) & 0xFF);
			DRV_PRINT( "  PHASE1_COUNT.............0x%02x (%d)\n", (xmii_config >> 8 ) & 0xFF,  (xmii_config >> 8 ) & 0xFF);
			DRV_PRINT( "  PHASE0_COUNT.............0x%02x (%d)\n", (xmii_config >> 8 ) & 0xFF,  (xmii_config >> 8 ) & 0xFF);
			DRV_PRINT( "-------------------------------------------------------\n");
#endif // #ifdef CONFIGURABLE_GIGE
			return xmii_config;
		}
		{
		}
	}
}

int
athr8035_phy_speed(int unit)
{
    int status;
    athr8035_phy_t *phy = athr8035_phy_find(unit);
    int ii = 200;

    DRV_PRINT("%s: enter athr8035_phy_speed!\n", MODULE_NAME);

    if (!phy) {
        DRV_PRINT("%s: phy unit %d not found !\n", MODULE_NAME, unit);
       	return 0;
    }

    if (eth_debug > 0) {
        athr8035_phy_reg_dump(phy->mac_unit, phy->phy_addr);
    }

    do {
	    status = phy_reg_read(unit, phy->phy_addr, ATHR_PHY_SPEC_STATUS);
	    sysMsDelay(10);
    }while((!(status & ATHR_STATUS_RESOLVED)) && --ii);

    DRV_PRINT("%s status = 0x%08x\n", __func__, status);
    if(status & ATHR_STATUS_RESOLVED) {
	    DRV_PRINT( "%s RESOLVED\n", __func__);
    } else {
	    DRV_PRINT( "%s *NOT* RESOLVED\n", __func__);
    }
    if(status & (1<<5)) {
	    DRV_PRINT( "%s SMARTSPEED DOWNGRADE\n", __func__);
    }
    if(status & (1<<2)) {
	    DRV_PRINT( "%s RECEIVE PAUSE ENABLED\n", __func__);
    } else {
	    DRV_PRINT( "%s RECEIVE PAUSE DISABLED\n", __func__);
    }
    if(status & (1<<1)) {
	    DRV_PRINT( "%s POLARITY REVERSED\n", __func__);
    } else {
	    DRV_PRINT( "%s POLARITY NORMAL\n", __func__);
    }
    if(status & (1<<0)) {
	    DRV_PRINT( "%s JABBER\n", __func__);
    } else {
	    DRV_PRINT( "%s NO JABBER\n", __func__);
    }
    if(status & (1<<3)) {
	    DRV_PRINT( "%s TRANSMIT PAUSE ENABLED\n", __func__);
    } else {
	    DRV_PRINT( "%s TRANSMIT PAUSE DISABLED\n", __func__);
    }
    if(status & (1<<10)) {
	    DRV_PRINT( "%s LINK UP\n", __func__);
    } else {
	    DRV_PRINT( "%s LINK DOWN\n", __func__);
    }
    if(status & (1<<13)) {
	    DRV_PRINT( "%s FULL DUPLEX\n", __func__);
    } else {
	    DRV_PRINT( "%s HALF DUPLEX\n", __func__);
    }
    if(status & (1<<6)) {
	    DRV_PRINT( "%s MDI\n", __func__);
    } else {
	    DRV_PRINT( "%s MDIX\n", __func__);
    }
    status = ((status & ATHER_STATUS_LINK_MASK) >> ATHER_STATUS_LINK_SHIFT);
    switch(status) {
    case 0:
	    DRV_PRINT("%s speed is 10 Mbps\n", __func__);
	    return _10BASET;
    case 1:
	    DRV_PRINT("%s speed is 100 Mbps\n", __func__);
	    return _100BASET;
    case 2:
	    DRV_PRINT("%s speed is 1000 Mbps\n", __func__);
	    return _1000BASET;
    default:
	    DRV_PRINT("%s speed is unknown\n", __func__);
	    return _10BASET;
    }
}

void athr8035_phy_reg_dump(int mac_unit, int phy_unit)
{
  int i, status;

  for ( i = 0; i<0x1d ; i++) {
    status = phy_reg_read(mac_unit, phy_unit, i);
    DRV_PRINT("0x%02x=0x%04x  ", i, status);
    if ( (i%8) == 7) DRV_PRINT("\n");
  }
  DRV_PRINT("\n");
  i=0x0;
  phy_reg_write(mac_unit, phy_unit, 0x1D, i);
  status = phy_reg_read(mac_unit, phy_unit, 0x1E);
  DRV_PRINT("DEBUG 0x%02x: %04x\n", i, status);
  i=0x05;
  phy_reg_write(mac_unit, phy_unit, 0x1D, i);
  status = phy_reg_read(mac_unit, phy_unit, 0x1E);
  DRV_PRINT("DEBUG 0x%02x: %04x\n", i, status);
  i=0x0b;
  phy_reg_write(mac_unit, phy_unit, 0x1D, i);
  status = phy_reg_read(mac_unit, phy_unit, 0x1E);
  DRV_PRINT("DEBUG 0x%02x: %04x\n", i, status);
  i=0x10;
  phy_reg_write(mac_unit, phy_unit, 0x1D, i);
  status = phy_reg_read(mac_unit, phy_unit, 0x1E);
  DRV_PRINT("DEBUG 0x%02x: %04x\n", i, status);
  i=0x11;
  phy_reg_write(mac_unit, phy_unit, 0x1D, i);
  status = phy_reg_read(mac_unit, phy_unit, 0x1E);
  DRV_PRINT("DEBUG 0x%02x: %04x\n", i, status);
  i=0x12;
  phy_reg_write(mac_unit, phy_unit, 0x1D, i);
  status = phy_reg_read(mac_unit, phy_unit, 0x1E);
  DRV_PRINT("DEBUG 0x%02x: %04x\n", i, status);
}
