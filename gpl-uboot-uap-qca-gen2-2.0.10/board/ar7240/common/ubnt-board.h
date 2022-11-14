#ifndef __BOARD_H
#define __BOARD_H

#define __MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define __MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

#define BOARD_CPU_TYPE_AR7240 0
#define BOARD_CPU_TYPE_AR7241 1
#define BOARD_CPU_TYPE_AR7242 2
#define BOARD_CPU_TYPE_AR9330 3
#define BOARD_CPU_TYPE_AR9331 4
#define BOARD_CPU_TYPE_AR9341 5
#define BOARD_CPU_TYPE_AR9342 6
#define BOARD_CPU_TYPE_AR9344 7
#define BOARD_CPU_TYPE_MUSIC  8

#define	WIFI_TYPE_UNKNOWN	0xffff
#define	WIFI_TYPE_NORADIO	0x0000
#define	WIFI_TYPE_MERLIN	0x002a
#define	WIFI_TYPE_KITE		0x002b
#define	WIFI_TYPE_KIWI		0x002e
#define	WIFI_TYPE_OSPREY	0x0030

#define	HW_ADDR_COUNT 0x02

#define subsysid subdeviceid

typedef struct wifi_info {
	uint16_t type;
	uint16_t subvendorid;
	uint16_t subdeviceid;
	u8 hwaddr[6];
    uint16_t regdmn;
} wifi_info_t;

typedef struct eth_info {
	u8 hwaddr[6];
} eth_info_t;

#define BDINFO_MAGIC 0xDB1FACED

#define UNIFI_AR9344_RESET_BTN_GPIO 17
#define UNIFI_AR9342_RESET_BTN_GPIO 12
#define UNIFI_AR724x_RESET_BTN_GPIO 12
#define UNIFI_MUSIC_RESET_BTN_INT   20

typedef struct ubnt_bdinfo {
	u32 magic;
	u8 cpu_type;
	uint16_t cpu_revid;
	eth_info_t eth0;
	eth_info_t eth1;
    uint16_t boardid;
    uint16_t vendorid;
    uint32_t bomrev;
    int boardrevision;
    wifi_info_t wifi0;
    wifi_info_t wifi1;
    int reset_button_gpio;
    int is_led_gpio_invert;
    int led_gpio_1;
    int led_gpio_2;
} ubnt_bdinfo_t;

const ubnt_bdinfo_t* board_identify(ubnt_bdinfo_t* b);

u32 ar724x_get_pcicfg_offset(void);
const char* cpu_type_to_name(int);
void board_dump_ids(char*, const ubnt_bdinfo_t*);

#ifdef CAL_ON_LAST_SECTOR
#define WLANCAL_OFFSET		0x1000
#define BOARDCAL_OFFSET		0x0
u32 ar724x_get_bdcfg_addr(void);

u32 ar724x_get_wlancfg_addr(int);

u32 ar724x_get_bdcfg_sector(void);
#endif

#define SWAP16(_x) ( (unsigned short)( (((const unsigned char *)(&_x))[1] ) | ( ( (const unsigned char *)( &_x ) )[0]<< 8) ) )

#define MERLIN_EEPROM_OFFSET 0x200
#define MERLIN_PCI_SUBVENDORID_OFFSET 0x14
#define MERLIN_PCI_SUBSYSTEMID_OFFSET 0x16
#define MERLIN_PCIE_SUBVENDORID_OFFSET 0x26
#define MERLIN_PCIE_SUBSYSTEMID_OFFSET 0x28

#define KITE_EEPROM_OFFSET 0x80
#define KITE_PCIE_SUBVENDORID_OFFSET 0x14
#define KITE_PCIE_SUBSYSTEMID_OFFSET 0x16

#define KIWI_EEPROM_OFFSET 0x100
#define KIWI_PCI_SUBVENDORID_OFFSET 0x14
#define KIWI_PCI_SUBSYSTEMID_OFFSET 0x16
#define KIWI_PCIE_SUBVENDORID_OFFSET 0x26
#define KIWI_PCIE_SUBSYSTEMID_OFFSET 0x28

#define WLAN_MAC_OFFSET 0xC
#define WLAN_REGDMN_OFFSET 0x8
#define OSPREY_MAC_OFFSET 0x2
#define OSPREY_REGDMN_OFFSET 0x1B

#endif /* __BOARD_H */
