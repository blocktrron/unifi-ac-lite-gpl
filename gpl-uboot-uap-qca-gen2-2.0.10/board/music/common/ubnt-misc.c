
#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <config.h>
#include <version.h>
#include "music_soc.h"

#define GPIO1_BLUE_LED 		(0x1 << 1)
#define GPIO2_WHITE_LED 	(0x1 << 2)
#define INT_EXT_RESET_PAD 	0x18050000

enum {
	UNIFI_LED_OFF = 0x0,
	UNIFI_LED_BLUE,
	UNIFI_LED_WHITE,
	UNIFI_LED_WHITE_BLUE,
};

#if defined (CONFIG_MUSIC)
void unifi_set_led(int pattern)
{
	switch (pattern) {
		case UNIFI_LED_OFF:
			music_reg_wr(MUSIC_REG_GPIO_DATA_OUT, 0);
			break;
		case UNIFI_LED_BLUE:
			music_reg_wr(MUSIC_REG_GPIO_DATA_OUT, GPIO1_BLUE_LED);
			break;
		case UNIFI_LED_WHITE:
			music_reg_wr(MUSIC_REG_GPIO_DATA_OUT, GPIO2_WHITE_LED);
			break;
		case UNIFI_LED_WHITE_BLUE:
			music_reg_wr(MUSIC_REG_GPIO_DATA_OUT, GPIO1_BLUE_LED | GPIO2_WHITE_LED);
			break;
		default:
			break;
	}
}

inline unsigned int
gpio_input_get(u8 num)
{
	u32 reg_in;
	u32 reg_bit = 1 << num;

	reg_in =  music_reg_rd(INT_EXT_RESET_PAD); /* flush */

	return !((reg_in & reg_bit) >> num);
}

#endif
