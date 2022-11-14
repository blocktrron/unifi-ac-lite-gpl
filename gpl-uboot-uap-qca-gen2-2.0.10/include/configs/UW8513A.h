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
#define CFG_MAX_FLASH_SECT      0x2000 /*0x400*/    /* max number of sectors on one chip */
#define CFG_FLASH_SECTOR_SIZE   (4*1024)
#define CFG_FLASH_SIZE          0x00800000 /* Total flash size: 8M Bytes = 64M bits   modified for bz */
//#define CFG_FLASH_SIZE          0x00400000 /* Total flash size */
#endif


#if 0
#if (CFG_MAX_FLASH_SECT * CFG_FLASH_SECTOR_SIZE) != CFG_FLASH_SIZE
#	error "Invalid flash configuration"
#endif
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


#define	CONFIG_BOOTARGS     "console=ttyS0,115200 root=31:03 rootfstype=jffs2 init=/sbin/init" \
                    " mtdparts=music-nor0:256k(u-boot),64k(u-boot-env),2048k(image),5760k(rootfs) mem=32M"
/* default mtd partition table */
#undef MTDPARTS_DEFAULT
#define MTDPARTS_DEFAULT    "mtdparts=music-nor0:256k(u-boot),64k(u-boot-env),2688k(image),1024k(rootfs)"

#define CONFIG_BOOTFILE "/flash/music.img"
#define CONFIG_SECONDARY_BOOTFILE "/flash/music.2nd.img"
#define CONFIG_YAFFS2_START_ADDRESS "0x300000"
#define CONFIG_YAFFS2_SIZE          "0x6400000"

#define CONFIG_OEM_VENDOR           "QCA"
#define CONFIG_OEM_PRODUCT          "UW8513A"
#define CONFIG_OEM_BOARD_VER        "2"
#define CONFIG_OEM_BOARD_SERIAL     "7"
#define CONFIG_OEM_BOARD_MAC        "0x00:0x01:0x02:0x03:0x04:0x07"
#define CONFIG_ASDK_VER      		"0.9.7.25"

#define CONFIG_EXTRA_ENV_SETTINGS                   \
    "asdk_ver="CONFIG_ASDK_VER"\0"           		\
    "oem_vendor="CONFIG_OEM_VENDOR"\0"              \
    "oem_product="CONFIG_OEM_PRODUCT"\0"            \
    "oem_board_version="CONFIG_OEM_BOARD_VER"\0"    \
    "oem_board_serial="CONFIG_OEM_BOARD_SERIAL"\0"  \
    "oem_board_mac="CONFIG_OEM_BOARD_MAC"\0"        \
    "up_uboot=ping ${serverip}; tftp 0x80060000 "CONFIG_OEM_PRODUCT"_uboot_spi_v${asdk_ver}.bin;"  \
        " erase 0xb9000000 +0x40000; cp.b 0x80060000 0xb9000000 0x40000 \0"                 \
    "up_linux31=ping ${serverip}; tftp 0x80060000 "CONFIG_OEM_PRODUCT"_vmlinux.gz.uImage_2.6.31_v${asdk_ver}; "      \
        "erase 0xb9050000 +0x200000; cp.b 0x80060000 0xb9050000 0x200000\0"                                 \
    "up_fs31=ping ${serverip}; tftp 0x80060000 "CONFIG_OEM_PRODUCT"_spi_jffs2_linux_2.6.31_v${asdk_ver};"   \
        " erase 0xb9250000 +0x5a0000; cp.b 0x80060000 0xb9250000 0x5a0000 \0"                              \
    "up_linux19=ping ${serverip}; tftp 0x80060000 "CONFIG_OEM_PRODUCT"_vmlinux.gz.uImage_2.6.19.7_v${asdk_ver};"   \
        " erase 0xb9050000 +0x200000; cp.b 0x80060000 0xb9050000 0x200000\0"                                    \
    "up_fs19=ping ${serverip}; tftp 0x80060000 "CONFIG_OEM_PRODUCT"_spi_jffs2_linux_2.6.19.7_v${asdk_ver};"\
        " erase 0xb9250000 +0x5a0000; cp.b 0x80060000 0xb9250000 0x5a0000 \0"

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

#ifdef CONFIG_ATH_NAND_FL
#define CFG_ENV_SIZE		0x10000
#else
#define CFG_ENV_SECT_SIZE	CFG_FLASH_SECTOR_SIZE
//#define CFG_ENV_SIZE		0x400
#define CFG_ENV_SIZE		0x800
#endif

#define CFG_OEM_LEN    CFG_FLASH_SECTOR_SIZE

#ifdef CONFIG_ATH_NAND_FL
#define CONFIG_BOOTCOMMAND "bootm 0x80000"
#define CFG_OEM_ADDR		(0x00040000)
#define CFG_ENV_ADDR		(CFG_OEM_ADDR + CFG_OEM_LEN)
#else

#ifdef ASDK_SPI_BOOTROM_MODE
#define CFG_OEM_ADDR		(0xb9040000)
#define CFG_ENV_ADDR		(CFG_OEM_ADDR + CFG_OEM_LEN)
#define CONFIG_BOOTCOMMAND "bootm 0xb9050000"

#else
#define CFG_OEM_ADDR		(0xbf040000)
#define CFG_ENV_ADDR		(CFG_OEM_ADDR + CFG_OEM_LEN)
#define CONFIG_BOOTCOMMAND "bootm 0xbf050000"
#endif

#endif


/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CONFIG_COMMANDS	(( CONFIG_CMD_DFL | CFG_CMD_DHCP | CFG_CMD_ELF | CFG_CMD_PCI |	\
	CFG_CMD_MII | CFG_CMD_PING | CFG_CMD_NET | CFG_CMD_JFFS2 | CFG_CMD_ENV | CFG_CMD_I2C |	\
	CFG_CMD_FLASH | CFG_CMD_LOADS | CFG_CMD_RUN | CFG_CMD_LOADB | CFG_CMD_ELF))

#ifdef CONFIG_ATH_NAND_FL
#define CONFIG_JFFS2_NAND	1	/* jffs2 on nand support */
/*
 * NAND flash
 */
#define CFG_MAX_NAND_DEVICE	1
#define NAND_MAX_CHIPS		1
/*#define CFG_NAND_BASE	NAND_FLASH_BASE*/
#endif

#define CONFIG_NET_MULTI
#define CONFIG_MEMSIZE_IN_BYTES

#define CONFIG_IPADDR   192.168.1.10
#define CONFIG_SERVERIP 192.168.1.27
//#define CONFIG_ETHADDR 0x00:0xaa:0xbb:0xcc:0xdd:0xee
#define CFG_FAULT_ECHO_LINK_DOWN    1


#define CFG_BOOTM_LEN	(16 << 20) /* 16 MB */
#define DEBUG
#define CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2 "hush>"

#include <cmd_confdefs.h>

#endif	/* __MUSIC_BOARD_H */
