/*
 * This file contains the configuration parameters for the dbau1x00 board.
 */

#ifndef __MUSIC_BOARD_H
#define __MUSIC_BOARD_H

#include <configs/music.h>
/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#ifdef CONFIG_ATH_NAND_FL
#define CFG_MAX_FLASH_BANKS     1	    /* max number of memory banks */
#define CFG_MAX_FLASH_SECT      0x2000    /* max number of sectors on one chip */
#define CFG_FLASH_SECTOR_SIZE   (64*2048)
#define CFG_FLASH_SIZE          0x40000000 /* Total flash size */

#else
#define CFG_MAX_FLASH_BANKS     1	    /* max number of memory banks */
#define CFG_MAX_FLASH_SECT      128    /* max number of sectors on one chip */
#define CFG_FLASH_SECTOR_SIZE   (64*1024)
#define CFG_FLASH_SIZE          0x00800000 /* Total flash size */
#endif


#if (CFG_MAX_FLASH_SECT * CFG_FLASH_SECTOR_SIZE) != CFG_FLASH_SIZE
#	error "Invalid flash configuration"
#endif

#define CFG_FLASH_WORD_SIZE     unsigned short

/*
 * We boot from this flash
 */

#define NAND_FLASH_BASE         0x0

#ifdef ASDK_SPI_BOOTROM_MODE
#define SPI_FLASH_BASE          0xb9000000
#else
#define SPI_FLASH_BASE          0xbf000000
#endif

#ifdef CONFIG_ATH_NAND_FL
#define CFG_FLASH_BASE		    NAND_FLASH_BASE
#else
#define CFG_FLASH_BASE		    SPI_FLASH_BASE
#endif

/*
 * The following #defines are needed to get flash environment right
 */
#define	CFG_MONITOR_BASE	TEXT_BASE
#define	CFG_MONITOR_LEN		(192 << 10)

#undef CONFIG_BOOTARGS
/* XXX - putting rootfs in last partition results in jffs errors */
#define	CONFIG_BOOTARGS     "console=ttyS0,115200 root=31:02 rootfstype=jffs2 init=/sbin/init mtdparts=music-nor0:256k(u-boot),64k(u-boot-env),2752k(rootfs),896k(uImage),64k(NVRAM),64k(ART)"

/* default mtd partition table */
#undef MTDPARTS_DEFAULT
#define MTDPARTS_DEFAULT    "mtdparts=music-nor0:256k(u-boot),64k(u-boot-env),5120k(rootfs),1024k(uImage)"

#define CONFIG_BOOTFILE "/flash/music.img"
#define CONFIG_SECONDARY_BOOTFILE "/flash/music.2nd.img"
#define CONFIG_YAFFS2_START_ADDRESS "0x300000"
#define CONFIG_YAFFS2_SIZE          "0x6400000"


/*
 * timeout values are in ticks
 */
#define CFG_FLASH_ERASE_TOUT	(2 * CFG_HZ) /* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT	(2 * CFG_HZ) /* Timeout for Flash Write */

/*
 * Cache lock for stack
 */
#define CFG_INIT_SP_OFFSET	0x1000

#define	CFG_ENV_IS_IN_FLASH    1
#undef CFG_ENV_IS_NOWHERE


#define CFG_ENV_SIZE		0x10000
#ifdef CONFIG_ATH_NAND_FL
#define CONFIG_BOOTCOMMAND "bootm 0x80000"
#define CFG_ENV_ADDR		0x00040000
#else

#ifdef ASDK_SPI_BOOTROM_MODE
#define CFG_ENV_ADDR		0xb9040000
#define CONFIG_BOOTCOMMAND "bootm 0xb92f0000"
#else
#define CFG_ENV_ADDR		0xbf040000
#define CONFIG_BOOTCOMMAND "bootm 0xbf2f0000"
#endif

#endif

/* DDR init values */

#define CONFIG_NR_DRAM_BANKS	2
#define CFG_DDR_REFRESH_VAL     0x4f10
#define CFG_DDR_CONFIG_VAL      0xc7bc8cd0
#define CFG_DDR_MODE_VAL_INIT   0x133
#ifdef LOW_DRIVE_STRENGTH
#	define CFG_DDR_EXT_MODE_VAL    0x2
#else
#	define CFG_DDR_EXT_MODE_VAL    0x0
#endif
#define CFG_DDR_MODE_VAL        0x33

#define CFG_DDR_TRTW_VAL        0x1f
#define CFG_DDR_TWTR_VAL        0x1e

#define CFG_DDR_CONFIG2_VAL	 0x9dd0e6a8
#define CFG_DDR_RD_DATA_THIS_CYCLE_VAL  0x00ff

/* DDR2 Init values */
#define CFG_DDR2_EXT_MODE_VAL    0x402

#define CONFIG_NET_MULTI

#define CONFIG_MEMSIZE_IN_BYTES
#define CONFIG_PCI

/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CONFIG_COMMANDS	(( CONFIG_CMD_DFL | CFG_CMD_DHCP | CFG_CMD_ELF | CFG_CMD_PCI |	\
	CFG_CMD_MII | CFG_CMD_PING | CFG_CMD_NET | CFG_CMD_JFFS2 | CFG_CMD_ENV |	\
	CFG_CMD_FLASH | CFG_CMD_LOADS | CFG_CMD_RUN | CFG_CMD_LOADB | CFG_CMD_ELF ))

#ifdef CONFIG_ATH_NAND_FL
#define CONFIG_JFFS2_NAND	1	/* jffs2 on nand support */
/*
 * NAND flash
 */
#define CFG_MAX_NAND_DEVICE	1
#define NAND_MAX_CHIPS		1
/*#define CFG_NAND_BASE	NAND_FLASH_BASE*/

#endif

#define CONFIG_IPADDR   192.168.1.10
#define CONFIG_SERVERIP 192.168.1.27
//#define CONFIG_ETHADDR 0x00:0xaa:0xbb:0xcc:0xdd:0xee
#define CFG_FAULT_ECHO_LINK_DOWN    1


#define CFG_BOOTM_LEN	(16 << 20) /* 16 MB */
#define DEBUG
#define CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2 "hush>"

/*
** Parameters defining the location of the calibration/initialization
** information for the two Merlin devices.
** NOTE: **This will change with different flash configurations**
*/

#define WLANCAL                        0xbfff1000
#define BOARDCAL                       0xbfff0000
#define ATHEROS_PRODUCT_ID             137
#define CAL_SECTOR                     (CFG_MAX_FLASH_SECT - 1)




#include <cmd_confdefs.h>

#endif	/* __MUSIC_BOARD_H */
