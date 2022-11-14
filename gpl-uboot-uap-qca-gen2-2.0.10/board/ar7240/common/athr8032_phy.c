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
#include "athr8032_phy.h"
#include "../../../cpu/mips/ar7240/ag934x.h"


#define TRUE    1
#define FALSE   0

#define MODULE_NAME "ATHR8032"
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
} athr8032_phy_t;

static athr8032_phy_t phy_info[] = {
   { is_enet_port: 1,
      mac_unit    : 0,
      phy_addr    : 0x01,
      id1         : ATHR_PHY_ID1_EXPECTED,
      id2         : ATHR_AR8032_PHY_ID2_EXPECTED },
};

static athr8032_phy_t*
athr8032_phy_find(int unit) {
   unsigned int i;
   athr8032_phy_t *phy;
   u32 id1, id2;

   for (i = 0; i < sizeof(phy_info) / sizeof(athr8032_phy_t); i++) {
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

static int getenv_int(const char *var) {
   const char *val = getenv(var);
   if (!val) return 0;
   return simple_strtol(val, NULL, 10);
}

/*static int athr8032_phy_found = 0;

BOOL
athr8032_found()
{
	return athr8032_phy_found;
}*/

static int athr8032_gpio_initialized = 0;
inline void
athr8032_configure_gpio(void){
    /*
    GPIO  0 - Output - RST# (Reset PHY, Set LOW to reset) (from CPU to PHY)
    GPIO  3 - Input  - INTP (Interrupt Status) (from PHY to CPU)
    GPIO  4 - Input  - ETH_CRS (from PHY to CPU)
    GPIO 12 - Input  - ETH_ERR (from PHY to CPU)
    GPIO 15 - Input  - ETH_COL (from PHY to CPU)
    GPIO 11 - Output - SIG1 (D25)
    GPIO 13 - Output - SIG3 (D27)
    GPIO 14 - Output - SIG4 (D28)
    */
    unsigned int signal_leds_mask = (1<<13)|(1<<14);

    if (athr8032_gpio_initialized) {
        return;
    }

    // 1. Disabling CLK_OBS5, since it can be output on GPIO4, which we use for ETH_CRS
   	ar7240_reg_rmw_clear(GPIO_FUNCTION_ADDRESS, GPIO_FUNCTION_CLK_OBS5_ENABLE);
	// 2. Disable JTAG so we can use GPIO 0 and GPIO 3
	ar7240_reg_rmw_set(GPIO_FUNCTION_ADDRESS, GPIO_FUNCTION_DISABLE_JTAG);
	// 3. Select GPIO outputs to 0 (CPU controlled)
	ar7240_reg_rmw_clear(GPIO_OUT_FUNCTION0_ADDRESS, GPIO_OUT_FUNCTION0_ENABLE_GPIO_0_MASK);
    ar7240_reg_rmw_clear(GPIO_OUT_FUNCTION2_ADDRESS, GPIO_OUT_FUNCTION2_ENABLE_GPIO_11_MASK);
    ar7240_reg_rmw_clear(GPIO_OUT_FUNCTION3_ADDRESS, 0x00ffffff);
    ar7240_reg_rmw_clear(GPIO_OUT_FUNCTION3_ADDRESS, GPIO_OUT_FUNCTION3_ENABLE_GPIO_14_MASK);
	// 4. Set GPIO 0 output value to 1 (not in reset)
	ar7240_reg_wr(GPIO_CLEAR, (1<<0) | signal_leds_mask);
	// 5. Disable interrupts on all Ethernet related GPIOs, except INTP.
	ar7240_reg_rmw_clear(GPIO_INT, (1 << 0) | (1 << 4) | (1 << 15));
	// 6. Enable interrupts from PHY on GPIO 3
	ar7240_reg_rmw_set(GPIO_INT, (1 << 3));
	// 7. Set GPIO 3,4,12 to input
	ar7240_reg_rmw_set(GPIO_OE_ADDRESS, (1 << 3) | (1 << 4) | (1 << 15));
	//ar7240_reg_rmw_set(GPIO_OE_ADDRESS, (1 << 3) | (1 << 4) | (1 < 12));
	// 8. Set GPIO 0, and signal LEDs to output
	ar7240_reg_rmw_clear(GPIO_OE_ADDRESS, (1 << 0) | signal_leds_mask);
	// 9. Set ETH_CRS to GPIO GPIO 4, ETH_COL to GPIO 15, and ETH_ERR to GPIO 17
	ar7240_reg_wr(GPIO_IN_ENABLE2, 0x00040F11);
	// 10. Start in MII slave mode (PHY has the crystal and generates both Rx and Tx clocks)
	ar7240_reg_wr(AG7240_ETH_CFG, AG7240_ETH_CFG_MII_GE0 | AG7240_ETH_CFG_MII_GE0_SLAVE);
	// 11. Set GE0 MAC configuration2 to 10/100 mode, PAD/CRC enabled, with length checks enabled
	ar7240_reg_wr(0x19000004, ar7240_reg_rd(0x19000004) | (AG7240_MAC_CFG2_PAD_CRC_EN | AG7240_MAC_CFG2_LEN_CHECK | AG7240_MAC_CFG2_IF_10_100));
	// 12. Reset the AR8032.  We must hold reset at least 1ms
	ar7240_reg_wr(GPIO_CLEAR, (1<<0)); // Pull reset line low on AR8032
	udelay(1100);
	ar7240_reg_wr(GPIO_SET,   (1<<0)); // Pull reset line high on AR8032
	udelay(1100);
    athr8032_gpio_initialized = 1;
}

BOOL
athr8032_phy_probe(int unit) {
    athr8032_configure_gpio();
    if(athr8032_phy_find(unit) == NULL) {
        return FALSE;
    } else {
        DRV_PRINT("%s: unit=%d, AR8032 Detected\n", __FUNCTION__, unit);
        return TRUE;
    }
}

BOOL
athr8032_phy_setup(int unit) {
   athr8032_phy_t *phy = athr8032_phy_find(unit);
   uint16_t  phyHwStatus;
   uint16_t  timeout;
   uint16_t  v __attribute__((unused));

   eth_debug = getenv_int("ethdebug");

   DRV_PRINT("%s: enter athr8032_phy_setup.  MAC unit %d!\n", MODULE_NAME, unit);

   if (!phy) {
      printf("\n%s: ERROR: No PHY found for unit %d\n", MODULE_NAME, unit);
      return 0;
   }

   // Reset the AR8032.  We must hold reset at least 1ms
   ar7240_reg_wr(GPIO_CLEAR, (1<<0)); // Pull reset line low on AR8032
   udelay(1100);
   ar7240_reg_wr(GPIO_SET,   (1<<0)); // Pull reset line high on AR8032
   udelay(1100);


#ifdef DISABLE_POWER_SAVING
   // Disable power saving
   phy_reg_write(unit, phy->phy_addr, 0x1D, 0x29 /* power saving control */);
   v = phy_reg_read(unit, phy->phy_addr, 0x1E);
   phy_reg_write(unit, phy->phy_addr, 0x1D, 0x29 /* power saving control */);
   phy_reg_write(unit, phy->phy_addr, 0x1E, v & ~(1 << 15) /* try to set TOP_PS_EN to OFF */);
   phy_reg_write(unit, phy->phy_addr, 0x1D, 0x29 /* power saving control */);
   v = phy_reg_read(unit, phy->phy_addr, 0x1E);
#endif // #if DISABLE_POWER_SAVING

#ifdef DISABLE_SMART_SPEED
   // Disable smart speed
   phy_reg_write(unit, phy->phy_addr, ATHR_SMART_SPEED, 0x0);
#endif // #if DISABLE_SMART_SPEED

   phy_reg_write(unit, phy->phy_addr, ATHR_INTERRUPT_ENABLE, 0x0); // Disable all interrupts in u-boot driver
   phy_reg_write(unit, phy->phy_addr, ATHR_AUTONEG_ADVERT, ATHR_ADVERTISE_ALL);
   phy_reg_write(unit, phy->phy_addr, ATHR_PHY_CONTROL,
        phy_reg_read(unit, phy->phy_addr, ATHR_PHY_CONTROL) |
        ATHR_CTRL_SOFTWARE_RESET |
        ATHR_CTRL_AUTONEGOTIATION_ENABLE);
   sysMsDelay(100);

   /*
    * Wait up to 3 seconds for ALL associated PHYs to finish
    * autonegotiation.  The only way we get out of here sooner is
    * if ALL PHYs are connected AND finish autonegotiation.
    */
   for (timeout = 300; timeout; sysMsDelay(10), timeout--) {
      phyHwStatus = phy_reg_read(unit, phy->phy_addr, ATHR_PHY_CONTROL);

      if (!ATHR_RESET_DONE(phyHwStatus)) continue;
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

   athr8032_phy_speed(unit);

   return 1;
}

int
athr8032_phy_is_up(int unit) {
   int status;
   athr8032_phy_t *phy = athr8032_phy_find(unit);

   eth_debug = getenv_int("ethdebug");

   DRV_PRINT("%s: enter athr8032_phy_is_up!\n", MODULE_NAME);
   if (!phy) {
      DRV_PRINT("%s: phy unit %d not found !\n", MODULE_NAME, unit);
      return 0;
   }

   status = phy_reg_read(unit, phy->phy_addr, ATHR_PHY_SPEC_STATUS);

   if (status & ATHR_STATUS_LINK_PASS) {
      DRV_PRINT("%s: athr8032 Link up!\n", MODULE_NAME);
      return 1;
   }

   DRV_PRINT("%s: athr8032 Link down!\n", MODULE_NAME);
   return 0;
}

int
athr8032_phy_is_fdx(int unit) {
   int status;
   athr8032_phy_t *phy = athr8032_phy_find(unit);
   int ii = 200;

   DRV_PRINT("%s: enter athr8032_phy_is_fdx!\n", MODULE_NAME);

   if (!phy) {
      DRV_PRINT("%s: phy unit %d not found !\n", MODULE_NAME, unit);
      return 0;
   }

   do {
      status = phy_reg_read(unit, phy->phy_addr, ATHR_PHY_SPEC_STATUS);
      sysMsDelay(10);
   }
   while ((!(status & ATHR_STATUS_RESOLVED)) && --ii);
   status = !(!(status & ATHER_STATUS_FULL_DEPLEX));

   return (status);
}

void athr8032_phy_reg_dump(int mac_unit, int phy_unit);

int
athr8032_phy_speed(int unit) {
   athr8032_phy_t *phy = athr8032_phy_find(unit);
   int status;
   int ii = 500;

   DRV_PRINT("%s: enter athr8032_phy_speed!\n", MODULE_NAME);

   if (!phy) {
      printf("%s: ERROR: PHY unit %d not found !\n", MODULE_NAME, unit);
      return 0;
   }

   do {
      status = phy_reg_read(unit, phy->phy_addr, ATHR_PHY_SPEC_STATUS);
      sysMsDelay(10);
   }
   while ((!(status & ATHR_STATUS_RESOLVED)) && --ii);

   if (eth_debug > 0) {
      athr8032_phy_reg_dump(phy->mac_unit, phy->phy_addr);
   }

   DRV_PRINT("%s status = 0x%08x\n", __func__, status);
   if (status & ATHR_STATUS_RESOLVED) {
      DRV_PRINT("%s RESOLVED\n", __func__);
   } else {
      DRV_PRINT("%s *NOT* RESOLVED\n", __func__);
   }
   if (status & (1 << 5)) {
      DRV_PRINT("%s SMARTSPEED DOWNGRADE\n", __func__);
   }
   if (status & (1 << 2)) {
      DRV_PRINT("%s RECEIVE PAUSE ENABLED\n", __func__);
   } else {
      DRV_PRINT("%s RECEIVE PAUSE DISABLED\n", __func__);
   }
   if (status & (1 << 1)) {
      DRV_PRINT("%s POLARITY REVERSED\n", __func__);
   } else {
      DRV_PRINT("%s POLARITY NORMAL\n", __func__);
   }
   if (status & (1 << 0)) {
      DRV_PRINT("%s JABBER\n", __func__);
   } else {
      DRV_PRINT("%s NO JABBER\n", __func__);
   }
   if (status & (1 << 3)) {
      DRV_PRINT("%s TRANSMIT PAUSE ENABLED\n", __func__);
   } else {
      DRV_PRINT("%s TRANSMIT PAUSE DISABLED\n", __func__);
   }
   if (status & (1 << 10)) {
      DRV_PRINT("%s LINK UP\n", __func__);
   } else {
      DRV_PRINT("%s LINK DOWN\n", __func__);
   }
   if (status & (1 << 13)) {
      DRV_PRINT("%s FULL DUPLEX\n", __func__);
   } else {
      DRV_PRINT("%s HALF DUPLEX\n", __func__);
   }
   if (status & (1 << 6)) {
      DRV_PRINT("%s MDIX\n", __func__);
   } else {
      DRV_PRINT("%s MDI\n", __func__);
   }
   status = ((status & ATHER_STATUS_LINK_MASK) >> ATHER_STATUS_LINK_SHIFT);
   switch (status) {
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

void athr8032_phy_reg_dump(int mac_unit, int phy_unit) {
   int i, status;
   int ii = 200;

   for (i = 0; i < 0x1f; i++) {
      status = phy_reg_read(mac_unit, phy_unit, i);
      printf("0x%02x=0x%04x  ", i, status);
      if ((i % 8) == 7) printf("\n");
   }
   printf("\n");
   i = 0x12;
   phy_reg_write(mac_unit, phy_unit, 0x1D, i);
   status = phy_reg_read(mac_unit, phy_unit, 0x1E);
   printf("DEBUG 0x%02x: %04x [Test Config 10Base-T]\n", i, status);
   i = 0x10;
   phy_reg_write(mac_unit, phy_unit, 0x1D, i);
   status = phy_reg_read(mac_unit, phy_unit, 0x1E);
   printf("DEBUG 0x%02x: %04x [Test Config 100Base-T]\n", i, status);
   i = 0x0b;
   phy_reg_write(mac_unit, phy_unit, 0x1D, i);
   status = phy_reg_read(mac_unit, phy_unit, 0x1E);
   printf("DEBUG 0x%02x: %04x [Hibernate Control]\n", i, status);
   i = 0x29;
   phy_reg_write(mac_unit, phy_unit, 0x1D, i);
   status = phy_reg_read(mac_unit, phy_unit, 0x1E);
   printf("DEBUG 0x%02x: %04x [Power Saving Control]\n", i, status);
}
