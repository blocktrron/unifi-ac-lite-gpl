#ifndef UBNT_APP
#include <common.h>
#include <command.h>
#include <asm/addrspace.h>
#include <ubnt_config.h>
#include "ubnt-board.h"
#include "ubnt-misc.h"

extern flash_info_t flash_info[];	/* info for FLASH chips */

static ubnt_bdinfo_t ubnt_bdinfo = { .magic = 0 };
#define is_identified(bd) ((bd) && ((bd)->magic == BDINFO_MAGIC))
#define is_not_identified(bd) (!(bd) || ((bd)->magic != BDINFO_MAGIC))
#define mark_identified(bd) do { if ((bd)) { (bd)->magic = BDINFO_MAGIC; } } while (0)

typedef unsigned char enet_addr_t[6];

//#ifndef UBNT_APP
static int wifi_identify(wifi_info_t* wifi, int idx);

/* FIXME */
u32
ar724x_get_pcicfg_offset(void) {
	u32 off = 12;
	const ubnt_bdinfo_t* b = board_identify(NULL);
	if (b) {
		switch (b->wifi0.type) {
		case WIFI_TYPE_KITE:
		case WIFI_TYPE_OSPREY:
			off = 3;
			break;
		default:
			off = 12;
			break;
		}
	}
	return off;
}
//#endif /* #ifndef UBNT_APP */

#define ATH7K_OFFSET_PCI_VENDOR_ID 0x04
#define ATH7K_OFFSET_PCI_DEVICE_ID 0x05

#define ATH7K_OFFSET_PCI_SUBSYSTEM_VENDOR_ID 0x0A
#define ATH7K_OFFSET_PCI_SUBSYSTEM_DEVICE_ID 0x0B

#define ATH7K_OFFSET_PCIE_VENDOR_ID 0x0D
#define ATH7K_OFFSET_PCIE_DEVICE_ID 0x0E
#define ATH7K_OFFSET_PCIE_SUBSYSTEM_VENDOR_ID 0x13
#define ATH7K_OFFSET_PCIE_SUBSYSTEM_DEVICE_ID 0x14

#define ATH7K_OFFSET_MAC1 0x106
#define ATH7K_OFFSET_MAC2 0x107
#define ATH7K_OFFSET_MAC3 0x108


//#ifndef UBNT_APP
static inline uint16_t
eeprom_get(void* ee, int off) {
	return *(uint16_t*)((char*)ee + off*2);

}

static int
check_eeprom_crc(void *base, void* end) {
	u_int16_t sum = 0, data, *pHalf;
	u_int16_t lengthStruct, i;

	lengthStruct = SWAP16(*(u_int16_t*)base);

	if ((((unsigned int)base+lengthStruct) > ((unsigned int)end))
			|| (lengthStruct == 0) ) {
		return 0;
	}

	/* calc */
	pHalf = (u_int16_t *)base;

	for (i = 0; i < lengthStruct/2; i++) {
		data = *pHalf++;
		sum ^= SWAP16(data);
	}

	if (sum != 0xFFFF) {
		return 0;
	}

	return 1;
}

static int
wifi_ar928x_identify(wifi_info_t* wifi, unsigned long wlancal_addr, unsigned long wlancal_end) {
	int wifi_type = WIFI_TYPE_UNKNOWN;
	uint16_t eeoffset = MERLIN_EEPROM_OFFSET;
	uint16_t subdev_offset = ATH7K_OFFSET_PCIE_SUBSYSTEM_DEVICE_ID;
	uint16_t subvend_offset = ATH7K_OFFSET_PCIE_SUBSYSTEM_VENDOR_ID;

	/* safety check - ar928x calibration data ALWAYS starts with a55a */
	if (*(unsigned short *)wlancal_addr != 0xa55a) {
		if (wifi) wifi->type = WIFI_TYPE_UNKNOWN;
		return WIFI_TYPE_UNKNOWN;
	}

	if (check_eeprom_crc((void*)(wlancal_addr + KIWI_EEPROM_OFFSET), (void *)wlancal_end)) {
		wifi_type = WIFI_TYPE_KIWI;
		eeoffset = KIWI_EEPROM_OFFSET;
	} else if (check_eeprom_crc((void*)(wlancal_addr + KITE_EEPROM_OFFSET), (void *)wlancal_end)) {
		wifi_type = WIFI_TYPE_KITE;
		eeoffset = KITE_EEPROM_OFFSET;
		subdev_offset = ATH7K_OFFSET_PCI_SUBSYSTEM_DEVICE_ID;
		subvend_offset = ATH7K_OFFSET_PCI_SUBSYSTEM_VENDOR_ID;
	} else if (check_eeprom_crc((void*)(wlancal_addr + MERLIN_EEPROM_OFFSET), (void *)wlancal_end)) {
		wifi_type = WIFI_TYPE_MERLIN;
		eeoffset = MERLIN_EEPROM_OFFSET;
	}

	if (wifi) {
		wifi->type = wifi_type;
		wifi->subdeviceid = eeprom_get((void*)wlancal_addr, subdev_offset);
		wifi->subvendorid = eeprom_get((void*)wlancal_addr, subvend_offset);
		memcpy(wifi->hwaddr,
			(void*)(wlancal_addr + eeoffset + WLAN_MAC_OFFSET), 6);
        memcpy(&(wifi->regdmn),
            (void*)(wlancal_addr + eeoffset + WLAN_REGDMN_OFFSET), 2);
	}
	return wifi_type;
}


static int
wifi_ar93xx_identify(wifi_info_t* wifi,
		unsigned long wlancal_addr, unsigned long wlancal_end) {
	/* osprey based designs, including hornet/wasp have abandoned a55a signature
	 * when storing calibration data on flash. Even worse, PCI/PCIE initialization
	 * data, which contains vendor/device/subvendor/subsystem IDs are no longer
	 * stored on flash - it has been moved to OTP inside the chip.
	 * Calibration data checksum has also been omitted - doh!
	 * For the detection to be more reliable, we'd need to query PCI, but it may be
	 * not initialized yet! OTP could be reliable source, however that one is also
	 * available only after PCI initialization...
	 * since all of it is so complicated, let's just do some rudimentary guesses..
	 */
	unsigned char* start = (unsigned char*)wlancal_addr;

	/* let's assume osprey will always use eepromVersion >= 02 and
	 * templateVersion >= 02 */
	if ((*start < 0x02) || (*(start + 1) < 0x02))
		return WIFI_TYPE_UNKNOWN;


	wifi->type = WIFI_TYPE_OSPREY;
	memcpy(wifi->hwaddr, (void*)(start + OSPREY_MAC_OFFSET), 6);
    memcpy(&(wifi->regdmn), (void*)(start + OSPREY_REGDMN_OFFSET), 2);
	/* hardcode subvendor/subsystem for now
	 * TODO: investigate OTP access without PCI initialization..
	 */
	wifi->subvendorid = 0x0777;
	wifi->subdeviceid = 0xffff;

	return wifi->type;
}

#if 0
static int
nowifi_identify(wifi_info_t* wifi, unsigned long cal_addr, unsigned long cal_end) {
	/*skip 2 MAC addresses at the beginning of EEPROM*/
	unsigned long subdeviceid_addr = cal_addr + sizeof(enet_addr_t) * HW_ADDR_COUNT;
	wifi->type = WIFI_TYPE_UNKNOWN;

	wifi->subdeviceid = eeprom_get((void *)subdeviceid_addr,0);
	wifi->subvendorid = eeprom_get((void *)subdeviceid_addr,1);

	if (wifi->subvendorid == 0x0777) {
		wifi->type = WIFI_TYPE_NORADIO;
	}

	return wifi->type;
}
#endif

static int
wifi_identify(wifi_info_t* wifi, int idx) {
	int wifi_type = WIFI_TYPE_UNKNOWN;

	unsigned long wlancal_addr = WLANCAL(idx);
	/* unsigned long boardcal_addr = BOARDCAL;
	unsigned int sector_size = CFG_FLASH_SECTOR_SIZE;
	void* boardcal_end = (void*)(boardcal_addr + sector_size); */
	unsigned short signature = *(unsigned short *)wlancal_addr;

	switch (signature) {
	case 0xa55a:
		wifi_type = wifi_ar928x_identify(wifi, wlancal_addr, wlancal_addr + WLANCAL_SIZE);
		break;
	case 0xffff:
		wifi_type = WIFI_TYPE_UNKNOWN;
		if (wifi) {
			wifi->type = wifi_type;
		}
		break;
	default:
		wifi_type = wifi_ar93xx_identify(wifi, wlancal_addr, wlancal_addr + WLANCAL_SIZE);
		break;
	}

#ifdef DEBUG
	if (wifi_type == WIFI_TYPE_UNKNOWN)
		printf("Wifi %d is not calibrated\n", idx);
#endif

	return wifi_type;
}

static inline void
ar7240_gpio_config_output(int gpio)
{
#ifdef CONFIG_WASP_SUPPORT
    ar7240_reg_rmw_clear(AR7240_GPIO_OE, (1 << gpio));
#else
    ar7240_reg_rmw_set(AR7240_GPIO_OE, (1 << gpio));
#endif
}

static inline void
ar7240_gpio_config_input(int gpio)
{
#ifdef CONFIG_WASP_SUPPORT
    ar7240_reg_rmw_set(AR7240_GPIO_OE, (1 << gpio));
#else
    ar7240_reg_rmw_clear(AR7240_GPIO_OE, (1 << gpio));
#endif
}

const ubnt_bdinfo_t*
board_identify(ubnt_bdinfo_t* bd) {
	ubnt_bdinfo_t* b = (bd == NULL) ? &ubnt_bdinfo : bd;
	if (is_not_identified(b)) {
		int x;
		unsigned long boardcal_addr = BOARDCAL;

		memset(b, 0, sizeof(*b));
		/* cpu type detection */
#if !defined (CONFIG_MUSIC)
		b->cpu_revid =
			ar7240_reg_rd(AR7240_REV_ID) & AR7240_REV_ID_MASK;
		x = BOARD_CPU_TYPE_AR7240;
		if (is_ar7241())
			x = BOARD_CPU_TYPE_AR7241;
		if (is_ar7242())
			x = BOARD_CPU_TYPE_AR7242;
		if (is_ar9330())
			x = BOARD_CPU_TYPE_AR9330;
		if (is_ar9331())
			x = BOARD_CPU_TYPE_AR9331;
		if (is_ar9341())
			x = BOARD_CPU_TYPE_AR9341;
		if (is_ar9342())
			x = BOARD_CPU_TYPE_AR9342;
		if (is_ar9344())
			x = BOARD_CPU_TYPE_AR9344;

		b->cpu_type = x;
#else
		b->cpu_type = BOARD_CPU_TYPE_MUSIC;
		/* B0 chip, add detection here */
		b->cpu_revid = 0x2;
#endif

		/* ethernet MACs */
		memcpy(b->eth0.hwaddr, (unsigned char*)boardcal_addr, 6);
		memcpy(b->eth1.hwaddr, (unsigned char*)(boardcal_addr + 6), 6);
		memcpy(&(b->boardid), (unsigned char*)(boardcal_addr + 12), 2);
		memcpy(&(b->vendorid), (unsigned char*)(boardcal_addr + 14), 2);
        memcpy(&(b->bomrev), (unsigned char*)(boardcal_addr + 16), 4);

        b->boardid = ntohs(b->boardid);
        b->vendorid = ntohs(b->vendorid);
        b->bomrev = ntohl(b->bomrev);

        if (b->bomrev == 0xffffffff) {
            b->boardrevision = -1;
        } else {
            b->boardrevision = b->bomrev & 0xff;
        }

		/* wifi type detection */
		wifi_identify(&b->wifi0, 0);
        wifi_identify(&b->wifi1, 1);

#if !defined (CONFIG_MUSIC)
        if (b->vendorid != 0x0777) {
            if ((b->wifi0.type == WIFI_TYPE_MERLIN) || (b->wifi0.type == WIFI_TYPE_KITE) || (b->wifi0.type == WIFI_TYPE_KIWI)) {
                b->boardid = b->wifi0.subdeviceid;
                b->vendorid = b->wifi0.subvendorid;
            } else if ((b->wifi1.type == WIFI_TYPE_MERLIN) || (b->wifi1.type == WIFI_TYPE_KITE) || (b->wifi1.type == WIFI_TYPE_KIWI)) {
                b->boardid = b->wifi1.subdeviceid;
                b->vendorid = b->wifi1.subvendorid;
            }
            /* We may not need this */
            /*
            b->boardid = ntohs(b->boardid);
            b->vendorid = ntohs(b->vendorid);
            */
        }

		if (is_ar9344() || is_ar9341()) {
            b->reset_button_gpio = UNIFI_AR9344_RESET_BTN_GPIO;
            b->is_led_gpio_invert = 0;
            b->led_gpio_1 = 12;
            b->led_gpio_2 = 13;
        } else if (is_ar9342()) {
            b->reset_button_gpio = UNIFI_AR9342_RESET_BTN_GPIO;
            b->is_led_gpio_invert = 0;
            b->led_gpio_1 = 14;
            b->led_gpio_2 = 13;
        } else {
            b->reset_button_gpio = UNIFI_AR724x_RESET_BTN_GPIO;
            b->is_led_gpio_invert = 0;
            b->led_gpio_1 = 1;
            if ((b->vendorid == 0x0777) && (b->boardid == 0xe302)) {
                /* set EJTAG_DISABLE, for GPIO_7 */
                ar7240_reg_rmw_set(AR7240_GPIO_FUNC, 0x1);

                /* clear I2S_MCKEN and UART_RTS_CTS_EN, for GPIO_11 */
                ar7240_reg_rmw_clear(AR7240_GPIO_FUNC, 0x08000004);

                /* set gpio 0, 1, 7, 11 as output */
                ar7240_reg_rmw_set(AR7240_GPIO_OE, 0x883);

                /* clear gpio 0, 1, 7, 11 (to turn off all LEDs) */
                ar7240_reg_rmw_clear(AR7240_GPIO_OUT, 0x883);

                b->led_gpio_2 = 11;
            } else {
                b->led_gpio_2 = 0;
            }
        }

        ar7240_gpio_config_input(b->reset_button_gpio);
        ar7240_gpio_config_output(b->led_gpio_1);
        ar7240_gpio_config_output(b->led_gpio_2);
#else
		b->reset_button_gpio = UNIFI_MUSIC_RESET_BTN_INT;
#endif
		mark_identified(b);
	}

	return b;
}

void
board_dump_ids(char* out_buffer, const ubnt_bdinfo_t* bd) {
    char* chp;
    char buf[16];

    chp = out_buffer;

    sprintf(buf, "%04x", bd->boardid);
    strcpy(chp, buf);
    chp += strlen(buf);

    if (bd->boardrevision >= 0) {
        sprintf(buf, "-%d", bd->boardrevision);
        strcpy(chp, buf);
        chp += strlen(buf);
    }

    sprintf(buf, ".%04x", bd->cpu_revid);
    strcpy(chp, buf);
    chp += strlen(buf);

    if ((bd->wifi0.type != WIFI_TYPE_UNKNOWN) && (bd->wifi0.type != WIFI_TYPE_NORADIO)) {
        sprintf(buf, ".%04x", bd->wifi0.type);
        strcpy(chp, buf);
        chp += strlen(buf);
    }

    if ((bd->wifi1.type != WIFI_TYPE_UNKNOWN) && (bd->wifi1.type != WIFI_TYPE_NORADIO)) {
        sprintf(buf, ".%04x", bd->wifi1.type);
        strcpy(chp, buf);
        chp += strlen(buf);
    }
}

const char*
cpu_type_to_name(int type) {
	switch (type) {
	case BOARD_CPU_TYPE_AR7240:
		return "AR7240";
		break;
	case BOARD_CPU_TYPE_AR7241:
		return "AR7241";
		break;
	case BOARD_CPU_TYPE_AR7242:
		return "AR7242";
		break;
	case BOARD_CPU_TYPE_AR9330:
		return "AR9330";
		break;
	case BOARD_CPU_TYPE_AR9331:
		return "AR9331";
		break;
	case BOARD_CPU_TYPE_AR9341:
		return "AR9341";
		break;
	case BOARD_CPU_TYPE_AR9342:
		return "AR9342";
		break;
	case BOARD_CPU_TYPE_AR9344:
		return "AR9344";
		break;
	case BOARD_CPU_TYPE_MUSIC:
		return "MUSIC";
		break;
	default:
		return "unknown";
	}
	return NULL;
}

static const char*
wifi_type_to_name(int type) {
	switch (type) {
	case WIFI_TYPE_MERLIN:
		return "merlin";
		break;
	case WIFI_TYPE_KITE:
		return "kite";
		break;
	case WIFI_TYPE_KIWI:
		return "kiwi";
		break;
	case WIFI_TYPE_OSPREY:
		return "osprey";
		break;
	case WIFI_TYPE_UNKNOWN:
	default:
		return "unknown";
	}
	return NULL;
}

static int
do_boardinfo(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
	const ubnt_bdinfo_t* b = board_identify(NULL);
	UNUSED(cmdtp);
	UNUSED(flag);
	UNUSED(argc);
	UNUSED(argv);

	printf("CPU: %s (%04x)\n", cpu_type_to_name(b->cpu_type), b->cpu_revid);
    if ((b->wifi0.type != WIFI_TYPE_UNKNOWN) && (b->wifi0.type != WIFI_TYPE_NORADIO)) {
	    printf("wifi0: %s (%04x:%04x)\n",
			    wifi_type_to_name(b->wifi0.type),
			    b->wifi0.subvendorid, b->wifi0.subsysid);
    }
    if ((b->wifi1.type != WIFI_TYPE_UNKNOWN) && (b->wifi1.type != WIFI_TYPE_NORADIO)) {
    	printf("wifi1: %s (%04x:%04x)\n",
			    wifi_type_to_name(b->wifi1.type),
			    b->wifi1.subvendorid, b->wifi1.subsysid);
    }
	printf("eth0  HW addr: " __MACSTR "\n", __MAC2STR(b->eth0.hwaddr));
	printf("eth1  HW addr: " __MACSTR "\n", __MAC2STR(b->eth1.hwaddr));
    if ((b->wifi0.type != WIFI_TYPE_UNKNOWN) && (b->wifi0.type != WIFI_TYPE_NORADIO)) {
	    printf("wifi0 HW addr: " __MACSTR "\n", __MAC2STR(b->wifi0.hwaddr));
    }
    if ((b->wifi1.type != WIFI_TYPE_UNKNOWN) && (b->wifi1.type != WIFI_TYPE_NORADIO)) {
	    printf("wifi1 HW addr: " __MACSTR "\n", __MAC2STR(b->wifi1.hwaddr));
    }
	return 0;
}

U_BOOT_CMD(uinfo, 1, 1, do_boardinfo,
#ifdef DEBUG
"uinfo   - show board information", ""
#else
"", ""
#endif
);
#endif /* #ifndef UBNT_APP */
