#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <config.h>
#include <version.h>
#include "ar7240_soc.h"
#include "../common/ubnt-board.h"

#ifdef CONFIG_ATH_NAND_BR
#include <nand.h>
#endif

extern int wasp_ddr_initial_config(uint32_t refresh);
extern int ar7240_ddr_find_size(void);

#ifdef COMPRESSED_UBOOT
#	define prmsg(...)
#ifndef UBNT_APP
#       define args             char *s
#endif
#	define board_str(a)	do {			\
	char ver[] = "0";				\
	strcpy(s, "U-Boot " a "Wasp 1.");		\
	ver[0] += ar7240_reg_rd(AR7240_REV_ID) & 0xf;	\
	strcat(s, ver);					\
} while (0)
#else
#	define prmsg		printf
#ifndef UBNT_APP
#       define args             void
#endif
#	define board_str(a)	printf(a)
#endif

void
wasp_usb_initial_config(void)
{
#define unset(a)	(~(a))

	if ((ar7240_reg_rd(WASP_BOOTSTRAP_REG) & WASP_REF_CLK_25) == 0) {
		ar7240_reg_wr_nf(AR934X_SWITCH_CLOCK_SPARE,
			ar7240_reg_rd(AR934X_SWITCH_CLOCK_SPARE) |
			SWITCH_CLOCK_SPARE_USB_REFCLK_FREQ_SEL_SET(2));
	} else {
		ar7240_reg_wr_nf(AR934X_SWITCH_CLOCK_SPARE,
			ar7240_reg_rd(AR934X_SWITCH_CLOCK_SPARE) |
			SWITCH_CLOCK_SPARE_USB_REFCLK_FREQ_SEL_SET(5));
	}

	udelay(1000);
	ar7240_reg_wr(AR7240_RESET,
		ar7240_reg_rd(AR7240_RESET) |
		RST_RESET_USB_PHY_SUSPEND_OVERRIDE_SET(1));
	udelay(1000);
	ar7240_reg_wr(AR7240_RESET,
		ar7240_reg_rd(AR7240_RESET) &
		unset(RST_RESET_USB_PHY_RESET_SET(1)));
	udelay(1000);
	ar7240_reg_wr(AR7240_RESET,
		ar7240_reg_rd(AR7240_RESET) &
		unset(RST_RESET_USB_PHY_ARESET_SET(1)));
	udelay(1000);
	ar7240_reg_wr(AR7240_RESET,
		ar7240_reg_rd(AR7240_RESET) &
		unset(RST_RESET_USB_HOST_RESET_SET(1)));
	udelay(1000);
	if ((ar7240_reg_rd(AR7240_REV_ID) & 0xf) == 0) {
		/* Only for WASP 1.0 */
		ar7240_reg_wr(0xb8116c84 ,
			ar7240_reg_rd(0xb8116c84) & unset(1<<20));
	}
}

void wasp_gpio_config(void)
{
	/* disable the CLK_OBS on GPIO_4 and set GPIO4 as input */
	ar7240_reg_rmw_clear(GPIO_OE_ADDRESS, (1 << 4));
	ar7240_reg_rmw_clear(GPIO_OUT_FUNCTION1_ADDRESS, GPIO_OUT_FUNCTION1_ENABLE_GPIO_4_MASK);
	ar7240_reg_rmw_set(GPIO_OUT_FUNCTION1_ADDRESS, GPIO_OUT_FUNCTION1_ENABLE_GPIO_4_SET(0x80));
	ar7240_reg_rmw_set(GPIO_OE_ADDRESS, (1 << 4));
}

int
wasp_mem_config(void)
{
	extern void ath_ddr_tap_cal(void);
	unsigned int type, reg32;

	type = wasp_ddr_initial_config(CFG_DDR_REFRESH_VAL);

	ath_ddr_tap_cal();

#ifdef DEBUG
	prmsg("Tap value selected = 0x%x [0x%x - 0x%x]\n",
		ar7240_reg_rd(AR7240_DDR_TAP_CONTROL0),
		ar7240_reg_rd(0xbd007f10), ar7240_reg_rd(0xbd007f14));
#endif

	/* Take WMAC out of reset */
	reg32 = ar7240_reg_rd(AR7240_RESET);
	reg32 = reg32 &  ~AR7240_RESET_WMAC;
	ar7240_reg_wr_nf(AR7240_RESET, reg32);

#if !defined(CONFIG_ATH_NAND_BR)
	/* Switching regulator settings */
	ar7240_reg_wr_nf(0x18116c40, 0x633c8176); /* AR_PHY_PMU1 */
	ar7240_reg_wr_nf(0x18116c44, 0x10380000); /* AR_PHY_PMU2 */

	wasp_usb_initial_config();

#endif /* !defined(CONFIG_ATH_NAND_BR) */

	wasp_gpio_config();

	reg32 = ar7240_ddr_find_size();

	return reg32;
}

long int initdram(int board_type)
{
	return (wasp_mem_config());
}

#ifndef UBNT_APP
extern void unifi_set_led(int);
int	checkboard(args)
{
    const ubnt_bdinfo_t* b;
    char buf[32];

    buf[0] = 0x00;
    b = board_identify(NULL);
    unifi_set_led(0);
    board_dump_ids(buf, b);
    printf("Board: Ubiquiti Networks %s board (%s)\n",
        cpu_type_to_name(b->cpu_type),
        buf);
	return 0;
}
#endif
