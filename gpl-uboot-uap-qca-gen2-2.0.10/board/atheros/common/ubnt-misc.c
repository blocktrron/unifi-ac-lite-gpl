#include <common.h>
#include <command.h>
#include <asm/addrspace.h>
#include <malloc.h>
#include <version.h>
#include <net.h>
#ifndef QCA_11AC
#include "ar7240_soc.h"
#else
#include "atheros.h"
#endif
#include "ubnt-board.h"
#include "ubnt-misc.h"

#define UNIFI_DEBUG 0
#if UNIFI_DEBUG
    #define DEBUGF(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
    #define DEBUGF(fmt, ...) do {} while (0)
#endif

extern flash_info_t flash_info[];	/* info for FLASH chips */

typedef unsigned char enet_addr_t[6];

static unsigned char enet_macs[2][6] = {
		{ 0x00, 0x15, 0x6D, 0xFF, 0x00, 0x00 },
		{ 0x00, 0x15, 0x6D, 0xFF, 0x00, 0x01 }
};

static int
fix_eeprom_crc(void *base, void* end) {
	u_int16_t sum = 0, data, *pHalf, *pSum;
	u_int16_t lengthStruct, i;

	lengthStruct = SWAP16(*(u_int16_t*)base);

	if ((((unsigned int)base+lengthStruct) > ((unsigned int)end))
			|| (lengthStruct == 0) ) {
		return 0;
	}

	/* calc */
	pHalf = (u_int16_t *)base;

    /* clear crc */
    pSum = pHalf + 1;
    *pSum = 0x0000;

	for (i = 0; i < lengthStruct/2; i++) {
		data = *pHalf++;
		sum ^= SWAP16(data);
	}

    sum ^= 0xffff;
    *pSum = SWAP16(sum);

	return 1;
}

#ifndef UBNT_APP
static int
do_setmac(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
	unsigned char* macstr = 0;
	int i;
	unsigned char* eeprom_buff;
    unsigned char* dest;
	unsigned char hwaddr[sizeof(enet_addr_t)]; /*Hardware address storage*/
	flash_info_t* flinfo = &flash_info[0];
	unsigned int sector_size = flinfo->size / flinfo->sector_count;
	unsigned long boardcal_addr = BOARDCAL;
	u32 cal_sector = CAL_SECTOR;
	UNUSED(cmdtp);UNUSED(flag);
    int wifi0mac_changed = 0;
    int wifi1mac_changed = 0;
    const ubnt_bdinfo_t* b;

    b = board_identify(NULL);

	if (argc < 2) {
		/* no args - show current programmed mac addresses */
		unsigned char* ptr = (unsigned char*)boardcal_addr;

		printf("Currently programmed:\n");
		printf("\tMAC0: " __MACSTR "\n", __MAC2STR(ptr));
		printf("\tMAC1: " __MACSTR "\n", __MAC2STR(ptr + 6));
        if ((b->wifi0.type != WIFI_TYPE_UNKNOWN) && (b->wifi0.type != WIFI_TYPE_NORADIO)) {
            printf("\tWIFI0: " __MACSTR "\n", __MAC2STR((unsigned char*)b->wifi0.hwaddr));
        }
        if ((b->wifi1.type != WIFI_TYPE_UNKNOWN) && (b->wifi1.type != WIFI_TYPE_NORADIO)) {
            printf("\tWIFI1: " __MACSTR "\n", __MAC2STR((unsigned char*)b->wifi1.hwaddr));
        }
		return 0;
	}

	if (!strcmp(argv[1], "-w")) {

        if ((b->wifi0.type == WIFI_TYPE_UNKNOWN) || (b->wifi0.type == WIFI_TYPE_NORADIO)) {
            printf("UNKNOWN WLAN CHIP.\n");
            return -1;
        }

		if (!memcmp(b->wifi0.hwaddr, "\x00\x00\x00\x00\x00\x00", 6)) {
			printf("WLAN MAC not found. Invalid EEPROM.\n");
			return -1;
		}

        memcpy((void *)hwaddr, b->wifi0.hwaddr, sizeof(hwaddr));
        hwaddr[3] -= 1;

        if (!memcmp((void *)boardcal_addr, (void *)hwaddr, sizeof(hwaddr))) {
            return 0;
        } else {
            printf("Using WLAN MAC " __MACSTR "\n", __MAC2STR(b->wifi0.hwaddr));
            memcpy((void *)enet_macs[0], b->wifi0.hwaddr, sizeof(enet_macs[0]));
        }
	} else {
		macstr = (unsigned char*)argv[1];
		for (i = 0; i < 6; i++) {
			if (!_is_hex(macstr[0]) || !_is_hex(macstr[1]))
				break;
			hwaddr[i] = (_from_hex(macstr[0]) * 16) + _from_hex(macstr[1]);
			macstr += 2;
			if (*macstr == ':')
				macstr++;
		}
		if (i != 6 || *macstr != '\0') {
			printf("Invalid MAC address\n");
			return -1;
		}
		memcpy(&enet_macs[0], &hwaddr, sizeof(enet_addr_t));
	}

    memcpy(&enet_macs[1], &enet_macs[0], sizeof(enet_addr_t));
    enet_macs[1][0] |= 0x02; /*Add "locally administered" flag*/

    memcpy((void *)hwaddr, enet_macs[0], sizeof(hwaddr));
    hwaddr[3] += 1;
    if ((b->wifi0.type == WIFI_TYPE_UNKNOWN) || (b->wifi0.type == WIFI_TYPE_NORADIO) || (!memcmp((void *)b->wifi0.hwaddr, (void *)hwaddr, sizeof(hwaddr)))) {
        wifi0mac_changed = 0;
    } else {
        memcpy((void *)b->wifi0.hwaddr, (void *)hwaddr, sizeof(b->wifi0.hwaddr));
        wifi0mac_changed = 1;
    }

    memcpy((void *)hwaddr, enet_macs[0], sizeof(hwaddr));
    hwaddr[3] += 2;
    if ((b->wifi1.type == WIFI_TYPE_UNKNOWN) || (b->wifi1.type == WIFI_TYPE_NORADIO) || (!memcmp((void *)b->wifi1.hwaddr, (void *)hwaddr, sizeof(hwaddr)))) {
        wifi1mac_changed = 0;
    } else {
        memcpy((void *)b->wifi1.hwaddr, (void *)hwaddr, sizeof(b->wifi1.hwaddr));
        wifi1mac_changed = 1;
    }

	eeprom_buff = malloc(sector_size);
	if (!eeprom_buff) {
		printf ("Out of memory\n");
		return -1;
	}

	printf ("Copying EEPROM from %p to %p\n",
			(void *)boardcal_addr, eeprom_buff);

	memcpy(eeprom_buff,(void *)boardcal_addr, sector_size);
	memcpy(eeprom_buff,(void *)enet_macs, sizeof(enet_macs));

	printf ("MAC0 %02X:%02X:%02X:%02X:%02X:%02X\n",
			eeprom_buff[0],
			eeprom_buff[1],
			eeprom_buff[2],
			eeprom_buff[3],
			eeprom_buff[4],
			eeprom_buff[5]);
	printf ("MAC1 %02X:%02X:%02X:%02X:%02X:%02X\n",
			eeprom_buff[6],
			eeprom_buff[7],
			eeprom_buff[8],
			eeprom_buff[9],
			eeprom_buff[10],
			eeprom_buff[11]);

    if (wifi0mac_changed) {
        switch (b->wifi0.type) {
            case WIFI_TYPE_KIWI:
                dest = eeprom_buff + WLANCAL_OFFSET + KIWI_EEPROM_OFFSET;
                memcpy(dest + WLAN_MAC_OFFSET, b->wifi0.hwaddr, sizeof(b->wifi0.hwaddr));
                fix_eeprom_crc(dest, dest + WLANCAL_SIZE);
                break;
            case WIFI_TYPE_KITE:
                dest = eeprom_buff + WLANCAL_OFFSET + KITE_EEPROM_OFFSET;
                memcpy(dest + WLAN_MAC_OFFSET, b->wifi0.hwaddr, sizeof(b->wifi0.hwaddr));
                fix_eeprom_crc(dest, dest + WLANCAL_SIZE);
                break;
            case WIFI_TYPE_MERLIN:
                dest = eeprom_buff + WLANCAL_OFFSET + MERLIN_EEPROM_OFFSET;
                memcpy(dest + WLAN_MAC_OFFSET, b->wifi0.hwaddr, sizeof(b->wifi0.hwaddr));
                fix_eeprom_crc(dest, dest + WLANCAL_SIZE);
                break;
            case WIFI_TYPE_OSPREY:
                dest = eeprom_buff + WLANCAL_OFFSET;
                memcpy(dest + OSPREY_MAC_OFFSET, b->wifi0.hwaddr, sizeof(b->wifi0.hwaddr));
                break;
        }
        printf ("WIFI0.MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
            b->wifi0.hwaddr[0],
            b->wifi0.hwaddr[1],
            b->wifi0.hwaddr[2],
            b->wifi0.hwaddr[3],
            b->wifi0.hwaddr[4],
            b->wifi0.hwaddr[5]);
    }

    if (wifi1mac_changed) {
        switch (b->wifi1.type) {
            case WIFI_TYPE_KIWI:
                dest = eeprom_buff + WLANCAL_OFFSET + WLANCAL_SIZE + KIWI_EEPROM_OFFSET;
                memcpy(dest + WLAN_MAC_OFFSET, b->wifi1.hwaddr, sizeof(b->wifi1.hwaddr));
                fix_eeprom_crc(dest, dest + WLANCAL_SIZE);
                break;
            case WIFI_TYPE_KITE:
                dest = eeprom_buff + WLANCAL_OFFSET + WLANCAL_SIZE + KITE_EEPROM_OFFSET;
                memcpy(dest + WLAN_MAC_OFFSET, b->wifi1.hwaddr, sizeof(b->wifi1.hwaddr));
                fix_eeprom_crc(dest, dest + WLANCAL_SIZE);
                break;
            case WIFI_TYPE_MERLIN:
                dest = eeprom_buff + WLANCAL_OFFSET + WLANCAL_SIZE + MERLIN_EEPROM_OFFSET;
                memcpy(dest + WLAN_MAC_OFFSET, b->wifi1.hwaddr, sizeof(b->wifi1.hwaddr));
                fix_eeprom_crc(dest, dest + WLANCAL_SIZE);
                break;
            case WIFI_TYPE_OSPREY:
                dest = eeprom_buff + WLANCAL_OFFSET + WLANCAL_SIZE;
                memcpy(dest + OSPREY_MAC_OFFSET, b->wifi1.hwaddr, sizeof(b->wifi1.hwaddr));
                break;
        }
        printf ("WIFI1.MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
            b->wifi1.hwaddr[0],
            b->wifi1.hwaddr[1],
            b->wifi1.hwaddr[2],
            b->wifi1.hwaddr[3],
            b->wifi1.hwaddr[4],
            b->wifi1.hwaddr[5]);
    }

	printf ("Erasing sector %d\n", cal_sector);
	flash_erase(flinfo, cal_sector, cal_sector);

	printf ("Writing EEPROM from %p to %p. [size - %04X]\n",
			eeprom_buff, (void *)boardcal_addr, sector_size);
	write_buff(flinfo, eeprom_buff, boardcal_addr, sector_size);

	free(eeprom_buff);
	printf ("Done.\n");
	return 0;
}

U_BOOT_CMD(usetmac, 2, 0, do_setmac,
#ifdef DEBUG
	"usetmac  - set/show ethernet MAC addresses\n",
	"[<XX:XX:XX:XX:XX:XX>] - program the MAC address into flash memory\n"
#else
	"", ""
#endif
);

static int
do_clearcal(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
	int reset_id, reset_eth;
	unsigned char* eeprom_buff;
	flash_info_t* flinfo = &flash_info[0];
	unsigned int sector_size = flinfo->size / flinfo->sector_count;
	unsigned long boardcal_addr = BOARDCAL;
	u32 cal_sector = CAL_SECTOR;
	UNUSED(cmdtp);UNUSED(flag);

    reset_id = 0;
    reset_eth = 0;
	while (argc > 1) {
		if (!(strcmp(argv[argc-1], "-f"))) {
            reset_id = 1;
#ifdef DEBUG
			printf("clearing IDs\n");
#endif
		}
		else if (!(strcmp(argv[argc-1], "-e"))) {
            reset_eth = 1;
#ifdef DEBUG
			printf("clearing Ethernet MAC address\n");
#endif
		}
		argc--;
	}

	eeprom_buff = malloc(sector_size);
	if (!eeprom_buff) {
		printf ("Out of memory\n");
		return -1;
	}

    memset(eeprom_buff, 0xff, sector_size);

    if (!reset_eth) {
#ifdef DEBUG
        printf("Copy MAC addresses from %p to %p\n", boardcal_addr, eeprom_buff);
#endif
        memcpy(eeprom_buff, (void *)boardcal_addr, 12);
    }

    if (!reset_id) {
#ifdef DEBUG
        printf("Copy IDs from %p to %p\n", boardcal_addr + 12, eeprom_buff + 12);
#endif
        memcpy(eeprom_buff + 12, (void *)(boardcal_addr + 12), 8);
    }

#ifdef DEBUG
	printf ("Erasing sector %d\n", cal_sector);
#endif
	flash_erase(flinfo, cal_sector, cal_sector);

#ifdef DEBUG
	printf ("Writing EEPROM from %p to %p. [size - %04X]\n",
			eeprom_buff, (void *)boardcal_addr, sector_size);
#endif
	write_buff(flinfo, eeprom_buff, boardcal_addr, sector_size);

	free(eeprom_buff);
	printf ("Done.\n");
	return 0;
}

U_BOOT_CMD(uclearcal, 3, 0, do_clearcal,
#ifdef DEBUG
	"uclearcal  - clear calibration data sector\n",
#else
	"", ""
#endif
);

static int
_do_clear_ubootenv(void) {
	flash_info_t* flinfo = &flash_info[0];
	int sector_size = flinfo->size / flinfo->sector_count;
    int num_sector, s_first;

    num_sector = CFG_ENV_SIZE / sector_size;
    s_first = (CFG_ENV_ADDR - flinfo->start[0]) / sector_size;

#ifdef DEBUG
    printf ("Erasing sector %d..%d\n", s_first , s_first + num_sector - 1);
#endif
    flash_erase(flinfo, s_first, s_first + num_sector - 1);

	return 0;
}

#ifndef UBNT_CFG_PART_SIZE
#define UBNT_CFG_PART_SIZE  0x40000     /* 256k(cfg) */
#endif

static int
_do_clearcfg(void) {
	flash_info_t* flinfo = &flash_info[0];
	int sector_size = flinfo->size / flinfo->sector_count;
	//unsigned long cfg_addr = BOARDCAL - UBNT_CFG_PART_SIZE;
    int num_sector, s_first;

    num_sector = UBNT_CFG_PART_SIZE / sector_size;
    s_first = CAL_SECTOR - num_sector;

#ifdef DEBUG
    printf ("Erasing sector %d..%d\n", s_first , s_first + num_sector - 1);
#endif
    flash_erase(flinfo, s_first, s_first + num_sector - 1);

    return 0;
}

static int
do_clearcfg(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
	UNUSED(cmdtp);UNUSED(flag);
    return _do_clearcfg();
}

U_BOOT_CMD(uclearcfg, 1, 0, do_clearcfg,
#ifdef DEBUG
	"uclearcfg  - clear configuration \n",
#else
	"", ""
#endif
);

static int
do_setboardid(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
	unsigned short vendorid, boardid;
    unsigned char* bidstr = 0;
	unsigned char* eeprom_buff;
    unsigned char* wcal_data;
	flash_info_t* flinfo = &flash_info[0];
	unsigned int sector_size = flinfo->size / flinfo->sector_count;
	unsigned long boardcal_addr = BOARDCAL;
	u32 cal_sector = CAL_SECTOR;
    const ubnt_bdinfo_t* bd;
    int i;
    int force;
    int cur;
	UNUSED(cmdtp);UNUSED(flag);

    bd = board_identify(NULL);

	if (argc <= 1) {
        printf("Board ID: %hx\n", bd->boardid);
        return 0;
    }

    force = 0;
    cur = 1;
    if (argc > 2) {
		if (!(strcmp(argv[1], "-f"))) {
            force = 1;
            cur++;
#ifdef DEBUG
			printf("Force setting board ID!\n");
#endif
		} else {
            printf("Board ID: %hx\n", bd->boardid);
            return 0;
        }
	}

    boardid = 0x0000;
    bidstr = (unsigned char*)argv[cur];
    for (i = 0; i < 4; i++) {
        if (!_is_hex(bidstr[i])) break;
        boardid = (boardid << 4) | (_from_hex(bidstr[i]) & 0xf);
    }

    if (i != 4) {
        printf("Error scanning board id\n");
        return -2;
    }

    if (!force) {
        if ((bd->vendorid == 0x0777) && (bd->boardid != boardid)) {
            printf("Board ID is programmed as %hx already!\n", bd->boardid);
            return -3;
        }
    }

	eeprom_buff = malloc(sector_size);
	if (!eeprom_buff) {
		printf ("Out of memory\n");
		return -1;
	}

    memcpy((void *)&(bd->boardid), (void *)&boardid, sizeof(boardid));
    memcpy(eeprom_buff,(void *)boardcal_addr, sector_size);

    vendorid=ntohs(0x0777);
    boardid=ntohs(boardid);
    memcpy(eeprom_buff + 12, (void *)&boardid, sizeof(boardid));
    memcpy(eeprom_buff + 12 + sizeof(boardid), (void *)&vendorid, sizeof(vendorid));
    switch (bd->wifi0.type) {
        case WIFI_TYPE_KIWI:
            wcal_data = eeprom_buff + WLANCAL_OFFSET;
            memcpy(wcal_data + KIWI_PCI_SUBVENDORID_OFFSET, (void *)&(vendorid), sizeof(vendorid));
            memcpy(wcal_data + KIWI_PCI_SUBSYSTEMID_OFFSET, (void *)&(boardid), sizeof(boardid));
            memcpy(wcal_data + KIWI_PCIE_SUBVENDORID_OFFSET, (void *)&(vendorid), sizeof(vendorid));
            memcpy(wcal_data + KIWI_PCIE_SUBSYSTEMID_OFFSET, (void *)&(boardid), sizeof(boardid));
            break;
        case WIFI_TYPE_KITE:
            wcal_data = eeprom_buff + WLANCAL_OFFSET;
            memcpy(wcal_data + KITE_PCIE_SUBVENDORID_OFFSET, (void *)&(vendorid), sizeof(vendorid));
            memcpy(wcal_data + KITE_PCIE_SUBSYSTEMID_OFFSET, (void *)&(boardid), sizeof(boardid));
            break;
        case WIFI_TYPE_MERLIN:
            wcal_data = eeprom_buff + WLANCAL_OFFSET;
            memcpy(wcal_data + MERLIN_PCI_SUBVENDORID_OFFSET, (void *)&(vendorid), sizeof(vendorid));
            memcpy(wcal_data + MERLIN_PCI_SUBSYSTEMID_OFFSET, (void *)&(boardid), sizeof(boardid));
            memcpy(wcal_data + MERLIN_PCIE_SUBVENDORID_OFFSET, (void *)&(vendorid), sizeof(vendorid));
            memcpy(wcal_data + MERLIN_PCIE_SUBSYSTEMID_OFFSET, (void *)&(boardid), sizeof(boardid));
            break;
    }

    switch (bd->wifi1.type) {
        case WIFI_TYPE_KIWI:
            wcal_data = eeprom_buff + WLANCAL_OFFSET + WLANCAL_SIZE;
            memcpy(wcal_data + KIWI_PCI_SUBVENDORID_OFFSET, (void *)&(vendorid), sizeof(vendorid));
            memcpy(wcal_data + KIWI_PCI_SUBSYSTEMID_OFFSET, (void *)&(boardid), sizeof(boardid));
            memcpy(wcal_data + KIWI_PCIE_SUBVENDORID_OFFSET, (void *)&(vendorid), sizeof(vendorid));
            memcpy(wcal_data + KIWI_PCIE_SUBSYSTEMID_OFFSET, (void *)&(boardid), sizeof(boardid));
            break;
        case WIFI_TYPE_KITE:
            wcal_data = eeprom_buff + WLANCAL_OFFSET + WLANCAL_SIZE;
            memcpy(wcal_data + KITE_PCIE_SUBVENDORID_OFFSET, (void *)&(vendorid), sizeof(vendorid));
            memcpy(wcal_data + KITE_PCIE_SUBSYSTEMID_OFFSET, (void *)&(boardid), sizeof(boardid));
            break;
        case WIFI_TYPE_MERLIN:
            wcal_data = eeprom_buff + WLANCAL_OFFSET + WLANCAL_SIZE;
            memcpy(wcal_data + MERLIN_PCI_SUBVENDORID_OFFSET, (void *)&(vendorid), sizeof(vendorid));
            memcpy(wcal_data + MERLIN_PCI_SUBSYSTEMID_OFFSET, (void *)&(boardid), sizeof(boardid));
            memcpy(wcal_data + MERLIN_PCIE_SUBVENDORID_OFFSET, (void *)&(vendorid), sizeof(vendorid));
            memcpy(wcal_data + MERLIN_PCIE_SUBSYSTEMID_OFFSET, (void *)&(boardid), sizeof(boardid));
            break;
    }

#ifdef DEBUG
	printf ("Erasing sector %d\n", cal_sector);
#endif
	flash_erase(flinfo, cal_sector, cal_sector);

#ifdef DEBUG
	printf ("Writing EEPROM from %p to %p. [size - %04X]\n",
			eeprom_buff, (void *)boardcal_addr, sector_size);
#endif
	write_buff(flinfo, eeprom_buff, boardcal_addr, sector_size);

	free(eeprom_buff);
	printf ("Done.\n");
	return 0;
}

U_BOOT_CMD(usetbid, 3, 0, do_setboardid,
#ifdef DEBUG
	"usetbid  - set board ID\n",
#else
	"", ""
#endif
);

static int
do_setbomrevision(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
	unsigned int bom_rev, rev_part0, rev_part1;
	unsigned char* eeprom_buff;
    char *tmp;
	flash_info_t* flinfo = &flash_info[0];
	unsigned int sector_size = flinfo->size / flinfo->sector_count;
	unsigned long boardcal_addr = BOARDCAL;
	u32 cal_sector = CAL_SECTOR;
        const ubnt_bdinfo_t* bd;
	UNUSED(cmdtp);UNUSED(flag);

        bd = board_identify(NULL);

	if (argc <= 1) {
            printf("BOM Rev: %05d-%02d\n",
                    (bd->bomrev & 0x1ffff00) >> 8,
                    bd->bomrev & 0xff);
            return 0;
        }

        rev_part1 = simple_strtoul((const char *)argv[1], &tmp, 10);
        rev_part0 = simple_strtoul((const char *)++tmp, NULL, 10);

        bom_rev = htonl(rev_part1 << 8 | rev_part0);

        eeprom_buff = malloc(sector_size);
        if (!eeprom_buff) {
            printf ("Out of memory\n");
            return -1;
        }

        memcpy((void *)&(bd->bomrev), (void *)&bom_rev,
                sizeof(bom_rev));
        bom_rev = htonl(bom_rev);
        memcpy(eeprom_buff,(void *)boardcal_addr, sector_size);
        memcpy(eeprom_buff + 16, (void *)&bom_rev, sizeof(bom_rev));

#ifdef DEBUG
	printf ("Erasing sector %d\n", cal_sector);
#endif
	flash_erase(flinfo, cal_sector, cal_sector);

#ifdef DEBUG
	printf ("Writing EEPROM from %p to %p. [size - %04X]\n",
			eeprom_buff, (void *)boardcal_addr, sector_size);
#endif
	write_buff(flinfo, eeprom_buff, boardcal_addr, sector_size);

	free(eeprom_buff);
	printf ("Done.\n");
	return 0;
}

U_BOOT_CMD(usetbrev, 2, 0, do_setbomrevision,
#ifdef DEBUG
	"usetbrev  - set BOM Rev\n",
#else
	"", ""
#endif
);

#define EEPROM_UBNT_INVALID 0xff
#define EEPROM_UBNT_DSS 1
#define EEPROM_UBNT_RSA 2
#define EEPROM_UBNT_MAGIC "\xFF\xFF\xFF\xFF""91NT"
#define EEPROM_UBNT_SIZE (1024 * 8) //8kB
#define SSH_STR_LEN 11
#define SSH_DSS_STR "\x00\x00\x00\x07""ssh-dss"
#define SSH_RSA_STR "\x00\x00\x00\x07""ssh-rsa"

typedef struct eeprom_entry {
    unsigned char id;
    unsigned short len;
    unsigned char* datap;
} eeprom_entry_t;

static int
do_setsshkey(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
	unsigned char* eeprom_buff;
	flash_info_t* flinfo = &flash_info[0];
	unsigned int sector_size = flinfo->size / flinfo->sector_count;
	unsigned long boardcal_addr = BOARDCAL;
	u32 cal_sector = CAL_SECTOR;
	ulong   addr, len;
    unsigned char* keyp;
    unsigned char* curp;
    eeprom_entry_t entry_input;
    eeprom_entry_t entry_table[2];
    eeprom_entry_t* entry_dssp;
    eeprom_entry_t* entry_rsap;
    int i, ret;
    unsigned short l;
	UNUSED(cmdtp);UNUSED(flag);

    entry_dssp = NULL;
    entry_rsap = NULL;
    entry_input.id = EEPROM_UBNT_INVALID;
    entry_input.datap = NULL;
    for (i = 0; i < 2; i++) {
        entry_table[i].id = EEPROM_UBNT_INVALID;
        entry_table[i].datap = NULL;
    }

    if (argc < 3) {
        return 1;
    }

	eeprom_buff = malloc(sector_size);
	if (!eeprom_buff) {
		printf ("Out of memory\n");
		return -1;
	}

	memcpy(eeprom_buff,(void *)boardcal_addr, sector_size);
    keyp = eeprom_buff + sector_size - EEPROM_UBNT_SIZE;

    if (memcmp(keyp, (void *)EEPROM_UBNT_MAGIC, strlen(EEPROM_UBNT_MAGIC)) == 0) {
        curp = keyp + 8;
        for (i = 0; i < 2; i++) {
            if ((*curp) != EEPROM_UBNT_INVALID) {
                entry_table[i].id = (*curp);
                memcpy((void *)&l, curp + 1, sizeof(l));
                entry_table[i].len = ntohs(l);
                entry_table[i].datap = malloc(entry_table[i].len);
                if (!entry_table[i].datap) {
                	printf ("Out of memory\n");
                    ret = -1;
                    goto sshkey_out;
                }
                memcpy(entry_table[i].datap, curp + 3, entry_table[i].len);
                if (entry_table[i].id == EEPROM_UBNT_DSS) {
                    entry_dssp = entry_table + i;
                } else if (entry_table[i].id == EEPROM_UBNT_RSA) {
                    entry_rsap = entry_table + i;
                }
                curp += 3 + entry_table[i].len;
            } else {
                break;
            }
        }
    }

    addr = simple_strtoul(argv[1], NULL, 16);
    len = simple_strtoul(argv[2], NULL, 16);
    if (memcmp((void *)addr, (void *)SSH_DSS_STR, SSH_STR_LEN) == 0) {
        entry_input.id = EEPROM_UBNT_DSS;
        entry_input.len = len;
        entry_input.datap = (void *)addr;
        entry_dssp = &entry_input;
    } else if (memcmp((void *)addr, (void *)SSH_RSA_STR, SSH_STR_LEN) == 0) {
        entry_input.id = EEPROM_UBNT_RSA;
        entry_input.len = len;
        entry_input.datap = (void *)addr;
        entry_rsap = &entry_input;
    } else {
        printf ("%08lx-%08lx doesn't contain valid key!\n", addr, addr + len);
        ret = -2;
        goto sshkey_out;
    }

    if ((entry_dssp == NULL) && (entry_rsap == NULL)) {
        ret = 0;
        goto sshkey_out;
    }

    memset(keyp, 0xff, EEPROM_UBNT_SIZE);

    curp = keyp;
    memcpy(curp, EEPROM_UBNT_MAGIC, strlen(EEPROM_UBNT_MAGIC));
    curp += 8;
    if ((entry_dssp) && (entry_dssp->id != EEPROM_UBNT_INVALID)) {
        *curp = entry_dssp->id;
        l = htons(entry_dssp->len);
        memcpy(curp + 1, (void *) &l, sizeof(l));
        memcpy(curp + 3, entry_dssp->datap, entry_dssp->len);
        curp += 3 + entry_dssp->len;
    }

    if ((entry_rsap) && (entry_rsap->id != EEPROM_UBNT_INVALID)) {
        *curp = entry_rsap->id;
        l = htons(entry_rsap->len);
        memcpy(curp + 1, (void *) &l, sizeof(l));
        memcpy(curp + 3, entry_rsap->datap, entry_rsap->len);
        curp += 3 + entry_rsap->len;
    }

#ifdef DEBUG
	printf ("Erasing sector %d\n", cal_sector);
#endif
	flash_erase(flinfo, cal_sector, cal_sector);

#ifdef DEBUG
	printf ("Writing EEPROM from %p to %p. [size - %04X]\n",
			eeprom_buff, (void *)boardcal_addr, sector_size);
#endif
	write_buff(flinfo, eeprom_buff, boardcal_addr, sector_size);

	printf ("Done.\n");
    ret = 0;

sshkey_out:
	if (eeprom_buff) free(eeprom_buff);
    for (i = 0; i < 2; i++) {
        if (entry_table[i].datap) free(entry_table[i].datap);
    }
	return ret;
}

U_BOOT_CMD(usetsshkey, 3, 0, do_setsshkey,
#ifdef DEBUG
	"usetsshkey  - set ssh key\n",
#else
	"", ""
#endif
);

static int
do_setregulatorydomain(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
    unsigned short set_country_code;
	unsigned short input_value;
    unsigned short reg_dmn;
	unsigned char* eeprom_buff;
    unsigned char* wcal_data;
	flash_info_t* flinfo = &flash_info[0];
	unsigned int sector_size = flinfo->size / flinfo->sector_count;
	unsigned long boardcal_addr = BOARDCAL;
	u32 cal_sector = CAL_SECTOR;
    const ubnt_bdinfo_t* bd;
    char buf[16];
    char* chp;
	UNUSED(cmdtp);UNUSED(flag);

    bd = board_identify(NULL);

	if (argc <= 1) {

        memset(buf, 0, 16);
        chp = buf;

        if ((bd->wifi0.type != WIFI_TYPE_UNKNOWN) && (bd->wifi0.type != WIFI_TYPE_NORADIO)) {
            sprintf(buf, "%04x", bd->wifi0.regdmn);
            chp = buf + strlen(buf);
        }

        if ((bd->wifi1.type != WIFI_TYPE_UNKNOWN) && (bd->wifi1.type != WIFI_TYPE_NORADIO)) {
            sprintf(chp, ".%04x", bd->wifi1.regdmn);
            chp = buf + strlen(buf);
        }

        if (chp != buf) {
            printf("RegulatoryDomain: %s\n", buf);
        }
        return 0;
    }

    if (argc > 3) {
        return 1;
    }

    if (argc == 3) {
        if (!(strcmp(argv[1], "-c"))) {
            set_country_code = 1;
            input_value = simple_strtoul(argv[2], NULL, 16);
        } else {
            return 1;
        }
    } else {
        set_country_code = 0;
        if (!(strcmp(argv[1], "-e"))) {
            input_value = 0x0000;
        } else {
            input_value = simple_strtoul(argv[1], NULL, 16);
        }
    }

    if (set_country_code) {
        reg_dmn = input_value & 0x3fff;
        reg_dmn = reg_dmn | (1 << 15);
    } else {
        reg_dmn = input_value & 0xffff;
    }

	eeprom_buff = malloc(sector_size);
	if (!eeprom_buff) {
		printf ("Out of memory\n");
		return -1;
	}

	memcpy(eeprom_buff,(void *)boardcal_addr, sector_size);

    if ((bd->wifi0.type != WIFI_TYPE_UNKNOWN) && (bd->wifi0.type != WIFI_TYPE_NORADIO)) {

        memcpy((void *)&(bd->wifi0.regdmn), (void *)&reg_dmn, sizeof(reg_dmn));

        switch (bd->wifi0.type) {
            case WIFI_TYPE_KIWI:
                wcal_data = eeprom_buff + WLANCAL_OFFSET + KIWI_EEPROM_OFFSET;
                memcpy(wcal_data + WLAN_REGDMN_OFFSET, &(bd->wifi0.regdmn), sizeof(bd->wifi0.regdmn));
                fix_eeprom_crc(wcal_data, wcal_data + WLANCAL_SIZE);
                break;
            case WIFI_TYPE_KITE:
                wcal_data = eeprom_buff + WLANCAL_OFFSET + KITE_EEPROM_OFFSET;
                memcpy(wcal_data + WLAN_REGDMN_OFFSET, &(bd->wifi0.regdmn), sizeof(bd->wifi0.regdmn));
                fix_eeprom_crc(wcal_data, wcal_data + WLANCAL_SIZE);
                break;
            case WIFI_TYPE_MERLIN:
                wcal_data = eeprom_buff + WLANCAL_OFFSET + MERLIN_EEPROM_OFFSET;
                memcpy(wcal_data + WLAN_REGDMN_OFFSET, &(bd->wifi0.regdmn), sizeof(bd->wifi0.regdmn));
                fix_eeprom_crc(wcal_data, wcal_data + WLANCAL_OFFSET);
                break;
            case WIFI_TYPE_OSPREY:
                wcal_data = eeprom_buff + WLANCAL_OFFSET;
                memcpy(wcal_data + OSPREY_REGDMN_OFFSET, &(bd->wifi0.regdmn), sizeof(bd->wifi0.regdmn));
                break;
        }
    }

    if ((bd->wifi1.type != WIFI_TYPE_UNKNOWN) && (bd->wifi1.type != WIFI_TYPE_NORADIO)) {

        memcpy((void *)&(bd->wifi1.regdmn), (void *)&reg_dmn, sizeof(reg_dmn));

        switch (bd->wifi1.type) {
            case WIFI_TYPE_KIWI:
                wcal_data = eeprom_buff + WLANCAL_OFFSET + WLANCAL_SIZE + KIWI_EEPROM_OFFSET;
                memcpy(wcal_data + WLAN_REGDMN_OFFSET, &(bd->wifi1.regdmn), sizeof(bd->wifi1.regdmn));
                fix_eeprom_crc(wcal_data, wcal_data + WLANCAL_SIZE);
                break;
            case WIFI_TYPE_KITE:
                wcal_data = eeprom_buff + WLANCAL_OFFSET + WLANCAL_SIZE + KITE_EEPROM_OFFSET;
                memcpy(wcal_data + WLAN_REGDMN_OFFSET, &(bd->wifi1.regdmn), sizeof(bd->wifi1.regdmn));
                fix_eeprom_crc(wcal_data, wcal_data + WLANCAL_SIZE);
                break;
            case WIFI_TYPE_MERLIN:
                wcal_data = eeprom_buff + WLANCAL_OFFSET + WLANCAL_SIZE + MERLIN_EEPROM_OFFSET;
                memcpy(wcal_data + WLAN_REGDMN_OFFSET, &(bd->wifi1.regdmn), sizeof(bd->wifi1.regdmn));
                fix_eeprom_crc(wcal_data, wcal_data + WLANCAL_OFFSET);
                break;
            case WIFI_TYPE_OSPREY:
                wcal_data = eeprom_buff + WLANCAL_OFFSET + WLANCAL_SIZE;
                memcpy(wcal_data + OSPREY_REGDMN_OFFSET, &(bd->wifi1.regdmn), sizeof(bd->wifi1.regdmn));
                break;
        }
    }

#ifdef DEBUG
	printf ("Erasing sector %d\n", cal_sector);
#endif
	flash_erase(flinfo, cal_sector, cal_sector);

#ifdef DEBUG
	printf ("Writing EEPROM from %p to %p. [size - %04X]\n",
			eeprom_buff, (void *)boardcal_addr, sector_size);
#endif
	write_buff(flinfo, eeprom_buff, boardcal_addr, sector_size);

	free(eeprom_buff);
	printf ("Done.\n");
	return 0;
}

U_BOOT_CMD(usetrd, 3, 0, do_setregulatorydomain,
#ifdef DEBUG
	"usetrd  - set Regulatory Domain\n",
#else
	"", ""
#endif
);

#endif /* UBNT_APP */

#ifdef CONFIG_JFFS2_CMDLINE
#include <jffs2/load_kernel.h>

static int
bad_image_at(ulong addr) {

    image_header_t header;
    image_header_t *hdr;
    ulong data, len, checksum;

    hdr = &header;
	/* Copy header so we can blank CRC field for re-calculation */
	memmove (&header, (char *)addr, sizeof(image_header_t));

	if (ntohl(hdr->ih_magic) != IH_MAGIC) {
#ifdef DEBUG
		puts ("Bad Magic Number\n");
#endif
		return 1;
	}

	data = (ulong)&header;
	len  = sizeof(image_header_t);

	checksum = ntohl(hdr->ih_hcrc);
	hdr->ih_hcrc = 0;

	if (crc32 (0, (uchar *)data, len) != checksum) {
#ifdef DEBUG
		puts ("Bad Header Checksum\n");
#endif
		return 1;
	}

	data = addr + sizeof(image_header_t);
	len  = ntohl(hdr->ih_size);

#ifdef DEBUG
	printf("   Verifying Checksum at 0x%p ...", data);
#endif
	if (crc32 (0, (uchar *)data, len) != ntohl(hdr->ih_dcrc)) {
#ifdef DEBUG
		printf ("Bad Data CRC\n");
#endif
		return 1;
	}
#ifdef DEBUG
	puts ("OK\n");
#endif
    return 0;
}

extern int mtdparts_init(void); /* common/cmd_jffs2.c */
extern int find_mtd_part(const char *id, struct mtd_device **dev,
		u8 *part_num, struct part_info **part); /* common/cmd_jffs2.c */
extern u32 jffs2_1pass_ls(struct part_info *part,const char *fname); /* fs/jffs2/jffs2_1pass.c */
extern u32 jffs2_1pass_load(char *dest, struct part_info *part,const char *fname); /* fs/jffs2/jffs2_1pass.c */
extern int do_bootm(cmd_tbl_t *, int, int, char *[]); /* common/cmd_bootm.c */

typedef struct ubnt_image_file {
    int bootid;
    char filename[32];
} ubnt_image_file_t;

static int
do_ubntfsboot(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
    unsigned long offset;
    char* partnameP;
    char* fnheadP;
    int check_only;

	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;

    char buffer[32];
    ubnt_image_file_t imagetable[3];
    ubnt_image_file_t *imageP;
    int max_image_num, image_idx, good_image_loaded;

//    int file_found, try_backup, boot_from, load_size, bad_image;
    int i;

    if (argc > 5 || argc < 4) {
        return 1;
    }

    if (argc == 5) {
        if (strcmp(argv[4], "-c")) {
            return 1;
        } else {
            check_only = 1;
        }
    } else {
        check_only = 0;
    }

    offset = simple_strtoul(argv[1], NULL, 16);
    partnameP = argv[2];
    fnheadP = argv[3];

    max_image_num = 0;
    image_idx = -1;
    good_image_loaded = 0;

    /* check partition, chpart if found, return if not found */
    if (mtdparts_init()) {
        printf("Error: mtdparts_init() failed! (check mtdparts variable?)\n");
        if (!check_only) run_command("urescue", 0);
        return 1;
    }

    if (!find_mtd_part(partnameP, &dev, &pnum, &part)) {
        printf("Error: partition %s not found on flash memory!\n", partnameP);
        if (!check_only) run_command("urescue", 0);
        return 1;
    }

    /* do we need this ?*/
	/*current_dev = dev;
	current_partnum = pnum;
	current_save();

	printf("partition changed to %s%d,%d\n",
			MTD_DEV_TYPE(dev->id->type), dev->id->num, pnum); */

    /* check file existence, return if not found */
#ifndef UBNT_FSBOOT_DUAL
    strcpy(buffer, fnheadP);
    if (jffs2_1pass_ls(part, buffer)) {
        strcpy(imagetable[max_image_num].filename, buffer);;
        imagetable[max_image_num].bootid = -1;
        max_image_num++;
    }
#else
    strcpy(buffer, fnheadP);
    i = strlen(buffer);
    buffer[i] = '0';
    buffer[i+1] = 0x00;
    if (jffs2_1pass_ls(part, buffer)) {
        strcpy(imagetable[max_image_num].filename, buffer);;
        imagetable[max_image_num].bootid = 0;
        max_image_num++;
    }

    strcpy(buffer, fnheadP);
    i = strlen(buffer);
    buffer[i] = '1';
    buffer[i+1] = 0x00;
    if (jffs2_1pass_ls(part, buffer)) {
        strcpy(imagetable[max_image_num].filename, buffer);;
        imagetable[max_image_num].bootid = 1;
        max_image_num++;
    }
#endif

    if (max_image_num < 1) {
        printf("Error: no image file found on partition: %s!\n", partnameP);
        if (!check_only) run_command("urescue", 0);
        return 1;
    }

    /* load & check crc, return if crc error */
    for (i = 0; i < max_image_num; i++) {
        imageP = imagetable + i;
#ifdef DEBUG
        printf("KMLUOH: checking %s ...... ", imageP->filename);
#endif
        if ((jffs2_1pass_load((char *)offset, part, imageP->filename) > 0) &&
            (!bad_image_at(offset))) {

            image_idx = i;
            good_image_loaded = 1;
#ifdef DEBUG
            printf("OK\n");
#endif
            break;
        }
#ifdef DEBUG
        printf("Failed\n");
#endif
    }

    if (!good_image_loaded) {
        printf("Error: none of the image file is good!\n");
        if (!check_only) run_command("urescue", 0);
        return 1;
    }

    if (check_only) {
        printf("%s:%s is loaded at 0x%08x successfully.\n", partnameP, imagetable[image_idx].filename, offset);
        return 0;
    }

    if (imagetable[image_idx].bootid > -1) {
        sprintf(buffer, "%d", imagetable[image_idx].bootid);
        setenv("ubntbootid", buffer);
    }

    do_bootm (cmdtp, flag, 2, argv);
	return 1;
}

U_BOOT_CMD(ubntfsboot, 5, 0, do_ubntfsboot,
	"ubntfsboot  - UBNT fsboot command\n",
);
#endif /* #ifdef CONFIG_JFFS2_CMDLINE */

#if 0
#ifndef PLL_MASK
#define PLL_MASK (PLL_CONFIG_DDR_DIV_MASK | PLL_CONFIG_AHB_DIV_MASK | PLL_CONFIG_PLL_NOPWD_MASK | PLL_CONFIG_PLL_REF_DIV_MASK | PLL_CONFIG_PLL_DIV_MASK)
#endif

#ifndef PLL_VAL
#define PLL_VAL(x,y,z) (PLL_CONFIG_##x##_##y##_##z##_PLL_REF_DIV_VAL|PLL_CONFIG_##x##_##y##_##z##_PLL_DIV_VAL|PLL_CONFIG_##x##_##y##_##z##_AHB_DIV_VAL|PLL_CONFIG_##x##_##y##_##z##_DDR_DIV_VAL|PLL_CONFIG_PLL_NOPWD_VAL)
#endif

#define pll_clr(_mask) do { \
	ath_reg_rmw_clear(ATH_CPU_PLL_CONFIG, _mask); \
} while (0)

#define clk_clr(_mask) do { \
	ath_reg_rmw_clear(ATH_CPU_CLOCK_CONTROL, _mask); \
} while (0)

#define pll_set(_mask, _val) do { \
	ath_reg_wr_nf(ATH_CPU_PLL_CONFIG, \
			(ath_reg_rd(ATH_CPU_PLL_CONFIG) & ~(_mask)) | _val); \
} while (0)

#define clk_set(_mask, _val) do { \
	ath_reg_wr_nf(ATH_CPU_CLOCK_CONTROL, \
			(ath_reg_rd(ATH_CPU_CLOCK_CONTROL) & ~(_mask)) | _val); \
} while (0)

typedef struct pll_info {
	uint32_t pll_cfg;
	char name[16];
} pll_info_t;

#define PLL_ENTRY(cpu,ddr,ahb) { PLL_VAL(cpu,ddr,ahb) , #cpu "/" #ddr "/" #ahb }

static pll_info_t pll_data[] = {
	PLL_ENTRY(420,420,210),
	PLL_ENTRY(400,400,200),
	PLL_ENTRY(400,400,100),
	PLL_ENTRY(400,200,200),
	PLL_ENTRY(390,390,195),
	PLL_ENTRY(360,360,180),
	PLL_ENTRY(360,360,90),
	PLL_ENTRY(350,350,175),
	PLL_ENTRY(340,340,170),
	PLL_ENTRY(330,330,165),
	PLL_ENTRY(320,320,160),
	PLL_ENTRY(320,320,80),
	PLL_ENTRY(310,310,155),
	PLL_ENTRY(300,300,150),
	PLL_ENTRY(300,300,75),
	PLL_ENTRY(240,240,120),
	PLL_ENTRY(200,200,100),
	PLL_ENTRY(160,160,80),
};


#define ALLOW_PLL_MODIFY

#ifdef ALLOW_PLL_MODIFY
static inline int
pll_modify(uint32_t val) {
	int x;
	void (*_restart)(void) = (void *)TEXT_BASE;

	printf(".");

	pll_clr(PLL_CONFIG_PLL_RESET_MASK);
	pll_set(PLL_MASK, val);
	pll_clr(PLL_CONFIG_PLL_BYPASS_MASK);
	/* wait for pll update */
	do {
		x = (ath_reg_rd(ATH_CPU_PLL_CONFIG) & PLL_CONFIG_PLL_UPDATE_MASK) >> PLL_CONFIG_PLL_UPDATE_SHIFT;
	} while (x);

	clk_set(CLOCK_CONTROL_RST_SWITCH_MASK, 0x2);
	clk_set(CLOCK_CONTROL_CLOCK_SWITCH_MASK, 0x1);
	_restart();

	return 0;
}
#endif

static int
do_pll(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
	uint32_t pll;
	uint32_t clk_ctrl;
	uint32_t i;
	pll_info_t* current;

	UNUSED(cmdtp);UNUSED(flag);

	pll = ath_reg_rd(ATH_CPU_PLL_CONFIG) & PLL_MASK;
	clk_ctrl = ath_reg_rd(ATH_CPU_CLOCK_CONTROL);

	current = NULL;
	for (i = 0; i < ARRAYSIZE(pll_data); ++i) {
		pll_info_t* pi = &pll_data[i];
		if (pi->pll_cfg == pll) {
			current = pi;
			break;
		}
	}

#if 0
	printf("Current PLL value: 0x%08x\n", pll);
	printf("Current Clock control value: 0x%08x; RST_SWITCH: %d\n", clk_ctrl, (clk_ctrl & CLOCK_CONTROL_RST_SWITCH_MASK) >> CLOCK_CONTROL_RST_SWITCH_SHIFT);
	printf("PLL mask: 0x%08x\n", PLL_MASK);
#endif
#ifdef ALLOW_PLL_MODIFY
	if (argc > 1 && !strcmp(argv[1], "-s")) {
		char* ep = NULL;
		int newval = -1;
		pll_info_t* new_pll = NULL;
		newval = simple_strtoul(argv[2], &ep, 10);
		if (ep && ep != argv[1] && *ep == '\0') {
			if ((newval >= 0) &&
					(newval < (int)ARRAYSIZE(pll_data)))
				new_pll = &pll_data[newval];
		}
		if (new_pll == NULL) {
			printf("Error: bad PLL ID (%s)!\n\n", argv[2]);
		} else if (current && (new_pll->pll_cfg == current->pll_cfg)) {
			printf("No changes to be made.\n");
		} else {
			printf("Changing from %s to %s (0x%08x)\n", current->name, new_pll->name, new_pll->pll_cfg);
			printf("\n");
			pll_modify(new_pll->pll_cfg);
			return 0;
		}
	}
#endif

	printf("Available settings:\n IDX\tCPU/DDR/AHB\tPLL\n");
	for (i = 0; i < ARRAYSIZE(pll_data); ++i) {
		pll_info_t* pi = &pll_data[i];
		printf("%3d%c\t%-12s\t%08x\n", i, (pi->pll_cfg == pll) ? '*' : ' ', pi->name, pi->pll_cfg);
	}
	return 0;
}

U_BOOT_CMD(pll, 3, 0, do_pll, "", "");

#define _GPIO_CR    ATH_GPIO_OE
#define _GPIO_DO    ATH_GPIO_OUT
#define _GPIO_DI    ATH_GPIO_IN

unsigned int
gpio_set(unsigned char* gpios, int count) {
	u32 reg_dir;
	u32 reg_out;
	int i;

	reg_dir = ath_reg_rd(_GPIO_CR);
	reg_out = ath_reg_rd(_GPIO_DO);

	/* toggle pins */
	for (i = 0; i < count; ++i) {
		u8 n = gpios[i] >> 2;
		u8 d =(gpios[i] >> 1) & 0x01;
		u8 o = gpios[i] & 0x01;

		if (d)
			reg_dir |= (d << n);
		else
			reg_dir &= ~(!d << n);

		if (o)
			reg_out |= (o << n);
		else
			reg_out &= ~(!o << n);
	}

	ath_reg_wr(_GPIO_CR, reg_dir);
	reg_dir = ath_reg_rd(_GPIO_CR); /* flush */

	ath_reg_wr(_GPIO_DO, reg_out);
	reg_out = ath_reg_rd(_GPIO_DO); /* flush */

	return reg_out;
}

/* rising-edge (aka positive-edge) bitbanging */
u32
gpio_bitbang_re(u8 clk_pin, u8 data_pin, u8 bitcount, u32 value) {
	u32 clk = 1 << clk_pin;
	u32 out = 1 << data_pin;
	u32 reg = ath_reg_rd(ATH_GPIO_OE);
	/* enable output on clk and data pins */
	if (!(reg & (clk | out))) {
		reg |= clk | out;
		ath_reg_wr_nf(ATH_GPIO_OE, reg);
		reg = ath_reg_rd(ATH_GPIO_OE); /* flush */
	}

	value <<= (32 - bitcount);
	ath_reg_wr_nf(ATH_GPIO_CLEAR, clk);

	while (bitcount) {
		if (value & (1 << 31))
			ath_reg_wr_nf(ATH_GPIO_SET, out);
		else
			ath_reg_wr_nf(ATH_GPIO_CLEAR, out);

		udelay(1);
		ath_reg_wr_nf(ATH_GPIO_SET, clk);
		udelay(1);
		value <<= 1;
		bitcount--;
		ath_reg_wr_nf(ATH_GPIO_CLEAR, clk);
	}
	return 0;
}
#endif /* 0 */



#if !defined(CONFIG_MUSIC)
void
unifi_set_led(int pattern)
{
    u32 reg_out;
    int led1, led2, invert=0;
#ifndef UBNT_APP
    const ubnt_bdinfo_t* bd;
    bd = board_identify(NULL);
    led1 = bd->led_gpio_1;
    led2 = bd->led_gpio_2;
    led2 = bd->led_gpio_2;
    invert = bd->is_led_gpio_invert;
#else
    char *env_led;

    env_led = getenv("led1");
    if (env_led)
        led1 = simple_strtoul(env_led, NULL, 10);

    env_led = getenv("led2");
    if (env_led)
        led2 = simple_strtoul(env_led, NULL, 10);

#endif

    reg_out = ath_reg_rd(ATH_GPIO_OUT) & (~((1 << led1) | (1 << led2)));
    if (invert) {
        switch (pattern) {
            case 0:
                ath_reg_wr(ATH_GPIO_OUT, reg_out | ((1 << led1) | (1 << led2)));
                break;
            case 1:
                ath_reg_wr(ATH_GPIO_OUT, reg_out | (1 << led2));
                break;
            case 2:
                ath_reg_wr(ATH_GPIO_OUT, reg_out | (1 << led1));
                break;
            case 3:
                ath_reg_wr(ATH_GPIO_OUT, reg_out);
                break;
            default:
                break;
        }
    } else {
        switch (pattern) {
            case 0:
                ath_reg_wr(ATH_GPIO_OUT, reg_out);
                break;
            case 1:
                ath_reg_wr(ATH_GPIO_OUT, reg_out | (1 << led1));
                break;
            case 2:
                ath_reg_wr(ATH_GPIO_OUT, reg_out | (1 << led2));
                break;
            case 3:
                ath_reg_wr(ATH_GPIO_OUT, reg_out | ((1 << led1) | (1 << led2)));
                break;
            default:
                break;
        }
    }
}
#endif

#ifndef UBNT_APP
#if !defined (CONFIG_MUSIC)
static inline unsigned int
gpio_input_get(u8 num) {
	u32 reg_in;
	u32 reg_bit = 1 << num;

	reg_in = ath_reg_rd(ATH_GPIO_IN); /* flush */

	return ((reg_in & reg_bit) >> num);
}
#endif

#endif /* UBNT_APP */

#if 0
#define GPIO_ON_MASK	0x01

static u8 gpio_data[] = {
	(0 << 2) | (GPIO_ON_MASK << 1),
};

static int
do_gpio(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
	unsigned int gpio_pin = 0;
	unsigned int state = 0;

	UNUSED(cmdtp);UNUSED(flag);

	/* display value if no argument */
	if (argc < 3) {
		printf ("Usage: gpio num state [0-off/1-on]\n");
		return -1;
	}

	gpio_pin = simple_strtoul(argv[1], NULL, 10);
	state = simple_strtoul(argv[2], NULL, 10);

	printf("GPIO %d into state: %s\n", gpio_pin, state ? "on" : "off");
	gpio_data[0] = (gpio_pin << 2) | (GPIO_ON_MASK << 1);
	if (state) {
		gpio_data[0] |= 1;
	} else {
		gpio_data[0] &= ~(1);
	}
	gpio_set(gpio_data, ARRAYSIZE(gpio_data));
	printf("GPIO dir=0x%08x; out=0x%08x\n",
			ath_reg_rd(_GPIO_CR), ath_reg_rd(_GPIO_DO));

	return 0;
}

U_BOOT_CMD(gpio, 3, 0, do_gpio,
#ifdef DEBUG
    "gpio    - GPIO control\n", "num state [0-off/1-on]\n"
#else
    "", ""
#endif
);


/* extra functions/commands for debugging */
#ifdef DEBUG
extern unsigned int s26_rd_phy(unsigned int phy_addr, unsigned int reg_addr);
extern void s26_wr_phy(unsigned int phy_addr, unsigned int reg_addr, unsigned int write_data);

static int
do_phyrd(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
	unsigned int phy;
	unsigned int reg;
	unsigned int value;

	if (argc < 3) {

		printf("usage:\n");
		printf("\tphyrd <phy> <reg>\n");
		return -1;
	}

	phy = simple_strtoul (argv[1], NULL, 16);
	reg = simple_strtoul(argv[2],NULL,16);

	value = s26_rd_phy(phy, reg);

	printf("Reading phy%x[0x%02x] -> 0x%08x\n", phy, reg, value);

	return 0;
}


U_BOOT_CMD(phyrd, 3, 0, do_phyrd,
#ifdef DEBUG
	"phyrd   - read PHY register\n", "\n"
#else
	"", ""
#endif
);

static int
do_phywr(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
	unsigned int phy;
	unsigned int reg;
	unsigned int value;

	if (argc < 4) {
		printf("usage:\n");
		printf("\tphyrd <phy> <reg> <val> \n");
		return -1;
	}

	phy = simple_strtoul (argv[1], NULL, 16);
	reg = simple_strtoul(argv[2],NULL,16);
	value = simple_strtoul(argv[3],NULL,16);

	printf("Writing phy%x[0x%02x] <- 0x%08x:\n", phy, reg, value);

	s26_wr_phy(phy, reg, value);

	return 0;
}

U_BOOT_CMD(phywr, 4, 0, do_phywr,
#ifdef DEBUG
	"phywr   - write PHY register\n", "\n"
#else
	"", ""
#endif
);
#endif
#endif /* 0 */

#ifndef UBNT_APP
#ifdef UNIFI_DETECT_RESET_BTN
static inline int poll_gpio_ms(int gpio, int duration, int freq, int revert) {
    int now;
    int cur_state, all_state;

    now = 0;
    all_state = revert ? (!gpio_input_get(gpio)) : gpio_input_get(gpio);
    while ((now < duration) && (all_state)) {
        cur_state = revert ? (!gpio_input_get(gpio)) : gpio_input_get(gpio);
        all_state &= cur_state;
        now += freq;
        udelay(freq * 1000);
    }

    return all_state;
}

int
handle_reset_button(void) {
    int reset_pressed;
    const ubnt_bdinfo_t* bd;

    bd = board_identify(NULL);

    reset_pressed = poll_gpio_ms(bd->reset_button_gpio, 3 * 1000, 50, 1);

    if (!reset_pressed) {
#ifdef DEBUG
        printf ("keep cfg partition. \n");
#endif
        return 0;
    }

    unifi_set_led(0);

    printf ("reset button pressed: clearing cfg partition!\n");
    unifi_set_led(3);
    _do_clearcfg();
    reset_pressed = poll_gpio_ms(bd->reset_button_gpio, 3 * 1000, 50, 1);
    unifi_set_led(0);

    if (!reset_pressed) {
        return 0;
    }

    reset_pressed = poll_gpio_ms(bd->reset_button_gpio, 3 * 1000, 50, 1);

    if (!reset_pressed) {
        return 0;
    }

    printf ("reset button pressed: clearing u-boot-env partition!\n");
    _do_clear_ubootenv();

    setenv("bootcmd", "urescue");

    return 1;
}
#endif

static int
do_setled(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) {
    int pattern;
	UNUSED(cmdtp);UNUSED(flag);

	if (argc <= 1) {
        return 0;
    }

    pattern = simple_strtol(argv[1], NULL, 10);

    unifi_set_led(pattern);

	return 0;
}

U_BOOT_CMD(usetled, 2, 0, do_setled,
#ifdef DEBUG
	"usetled  - set led pattern\n",
#else
	"", ""
#endif
);

#endif /* UBNT_APP */

#ifdef CAL_ON_LAST_SECTOR
static inline u32
ar724x_get_cfgsector_addr(void) {
	/* check whether flash_info is already populated */
	if ((u32)KSEG1ADDR(ATH_SPI_BASE) !=
			(u32)KSEG1ADDR(flash_info[0].start[0])) {
		/* TODO: this is true only for 64k sector size single SPI flash */
		return 0x9fff0000;
	}
	return (flash_info[0].start[0] + flash_info[0].size -
			   (flash_info[0].size / flash_info[0].sector_count));
}

u32
ar724x_get_bdcfg_addr(void) {
	return ar724x_get_cfgsector_addr() + BOARDCAL_OFFSET;
}

u32
ar724x_get_wlancfg_addr(int idx) {
	return ar724x_get_cfgsector_addr() + WLANCAL_OFFSET + (WLANCAL_SIZE * idx);
}

u32
ar724x_get_bdcfg_sector(void) {
	return flash_info[0].sector_count - 1;
}
#endif  /* #ifdef CAL_ON_LAST_SECTOR */

#ifdef UNIFI_FIX_ENV
#define MAX_UNIFI_BOOTARGS_BUFFER_SIZE  256
#define MAX_UNIFI_BOOTARGS_KEY_SIZE     16
static int
unifi_bootargs_append_if_not_found(char *key, char *val)
{
    char key_buffer[MAX_UNIFI_BOOTARGS_KEY_SIZE];
    char buffer[MAX_UNIFI_BOOTARGS_BUFFER_SIZE];
    size_t key_len, val_len;
    char *bootargs;
    char *key_ptr;
    int no_val, with_quote;

    if ((key == NULL) || (val == NULL))
        return 0;

    key_len = strlen(key);
    val_len = strlen(val);
    if (key_len == 0)
        return 0;

    if (val_len == 0) {
        no_val = 1;
    } else {
        no_val = 0;
    }

    /* for "=" */
    if (!no_val)
        key_len++;

    if ((key_len) >= MAX_UNIFI_BOOTARGS_KEY_SIZE) {
        printf("not enough buffer to store bootargs key\n");
        return 0;
    }

    bootargs = NULL;
    bootargs = getenv("bootargs");
    if ((bootargs == NULL) || (strlen(bootargs) == 0))
        return 0;

    memset(key_buffer, 0x00, MAX_UNIFI_BOOTARGS_KEY_SIZE);
    strcpy(key_buffer, key);
    if (!no_val)
        strcat(key_buffer, "=");

    key_ptr = strstr(bootargs, key_buffer);
    if (key_ptr == NULL) {
        DEBUGF("%s not found in bootargs, adding... \n", key_buffer);
        DEBUGF("bootargs = \"%s\", len = %d\n", bootargs, strlen(bootargs));
        if ((strlen(bootargs) + (key_len + 1) + val_len) < MAX_UNIFI_BOOTARGS_BUFFER_SIZE) {
            memset(buffer, 0x00, MAX_UNIFI_BOOTARGS_BUFFER_SIZE);
            if ((*bootargs == '"') && (*(bootargs + strlen(bootargs) - 1) == '"')) {
                with_quote = 1;
                strncpy(buffer, bootargs, strlen(bootargs) - 1);
            } else {
                with_quote = 0;
                strcpy(buffer, bootargs);
            }
            strcat(buffer, " ");
            strcat(buffer, key_buffer);
            if (!no_val)
                strcat(buffer, val);
            if (with_quote)
                strcat(buffer, "\"");
            setenv("bootargs", buffer);
            DEBUGF("new bootargs = \"%s\", len = %d\n", getenv("bootargs"), strlen(getenv("bootargs")));
            return 1;
        } else {
            printf("not enough buffer to fix bootargs\n");
            return 0;
        }
    } else {
        DEBUGF("%s found in bootargs, keep it untouched\n", key_buffer);
        DEBUGF("bootargs = \"%s\", len = %d\n", bootargs, strlen(bootargs));
        return 0;
    }
}

static int
unifi_fix_env_bootargs(void)
{
    char *bootargs = NULL;
    int fix_cnt = 0;

    bootargs = getenv("bootargs");
    if ((bootargs == NULL) || (strlen(bootargs) == 0)) {
        DEBUGF("bootargs not found or empty\n");
        setenv("bootargs", CONFIG_BOOTARGS);
        bootargs = getenv("bootargs");
        DEBUGF("new bootargs = %s\n", getenv("bootargs"));
        return 100;
    }

    fix_cnt += unifi_bootargs_append_if_not_found("console", BOOTARGS_DEFAULT_CONSOLE);

    fix_cnt += unifi_bootargs_append_if_not_found("panic", BOOTARGS_DEFAULT_PANIC_TIMEOUT);

    return fix_cnt;
}

extern void env_crc_update (void);

void
unifi_fix_env(void)
{
    int need_update = 0;
    need_update += unifi_fix_env_bootargs();
    if (need_update) {
        puts ("***           fixing environment variables ...\n");
        env_crc_update();
        saveenv();
    }
}
#endif  /* UNIFI_FIX_ENV */

DECLARE_GLOBAL_DATA_PTR;

int
unifi_init_gpio(void)
{
    if (is_ar9344() || is_ar9341()) {
        ath_reg_rmw_clear(ATH_GPIO_OE, (1 << 12));
        ath_reg_rmw_clear(ATH_GPIO_OE, (1 << 13));
        ath_reg_wr(ATH_GPIO_OUT, ath_reg_rd(ATH_GPIO_OUT) & (~((1 << 12) | (1 << 13))));
    } else if (is_ar9342()) {
        ath_reg_rmw_clear(ATH_GPIO_OE, (1 << 14));
        ath_reg_rmw_clear(ATH_GPIO_OE, (1 << 13));
        ath_reg_wr(ATH_GPIO_OUT, ath_reg_rd(ATH_GPIO_OUT) & (~((1 << 14) | (1 << 13))));
    } else {
        if (gd->ram_size < 0x4000000) {
            /* set EJTAG_DISABLE, for GPIO_7 */
            ath_reg_rmw_set(ATH_GPIO_FUNC, 0x1);

            /* clear I2S_MCKEN and UART_RTS_CTS_EN, for GPIO_11 */
            ath_reg_rmw_clear(ATH_GPIO_FUNC, 0x08000004);

            /* set gpio 0, 1, 7, 11 as output */
            ath_reg_rmw_set(ATH_GPIO_OE, 0x883);

            /* clear gpio 0, 1, 7, 11 (to turn off all LEDs) */
            ath_reg_rmw_clear(ATH_GPIO_OUT, 0x883);

            ath_reg_rmw_set(ATH_GPIO_OE, (1 << 1));
            ath_reg_rmw_set(ATH_GPIO_OE, (1 << 11));
            ath_reg_wr(ATH_GPIO_OUT, ath_reg_rd(ATH_GPIO_OUT) & (~((1 << 1) | (1 << 11))));
        } else {
            ath_reg_rmw_set(ATH_GPIO_OE, (1 << 1));
            ath_reg_rmw_set(ATH_GPIO_OE, (1 << 0));
            ath_reg_wr(ATH_GPIO_OUT, ath_reg_rd(ATH_GPIO_OUT) & (~((1 << 1) | (1 << 0))));
        }
    }
    return 0;
}

#ifdef UBNT_APP
int ubntappinit_found_in_env()
{
    char *ubntappinitp;

    ubntappinitp = NULL;
    ubntappinitp = getenv("ubntappinit");
    if ((ubntappinitp == NULL) || (strlen(ubntappinitp) == 0)) {
        return 0;
    } else {
        return 1;
    }
}
#endif /* #ifdef UBNT_APP */
