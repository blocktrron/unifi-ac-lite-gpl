/*
 * This file contains the configuration parameters for the usw-sw24p board.
 */

#ifndef __MUSIC_USW_SW24P_H
#define __MUSIC_USW_SW24P_H

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
#define CFG_MAX_FLASH_SECT      256 	/* max number of sectors on one chip */
#define CFG_FLASH_SECTOR_SIZE   (64*1024)
#define CFG_FLASH_SIZE          0x01000000 /* Total flash size: 16M Bytes = 128M bits */
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


/* default mtd partition table */
#undef MTDPARTS_DEFAULT
#define MTDPARTS_DEVICE  "music-nor0"
#define MTDPARTS_SQUASHFS_8	"mtdparts="MTDPARTS_DEVICE":256k(u-boot),64k(u-boot-env),1024k(kernel),6528k(rootfs),256k(cfg),64k(EEPROM)"
#define MTDPARTS_FSBOOT_DUAL_8 "mtdparts="MTDPARTS_DEVICE":256k(u-boot),64k(u-boot-env),7552k(jffs2),256k(cfg),64k(EEPROM)"
#define MTDPARTS_FSBOOT_DUAL_16 "mtdparts="MTDPARTS_DEVICE":256k(u-boot),64k(u-boot-env),15744k(jffs2),256k(cfg),64k(EEPROM)"
#define MTDPARTS_DEFAULT    MTDPARTS_FSBOOT_DUAL_16

#define EXTRA_ENV_SETTINGS_FSBOOT_DUAL      \
            "mtdparts="MTDPARTS_DEFAULT"\0"		\
            "ubntbootaddr=0x81000000\0"		\
            "ubntboot=ubntfsboot ${ubntbootaddr} jffs2 kernel\0"
#define EXTRA_ENV_SETTINGS_SQUASHFS         \
            "mtdparts="MTDPARTS_DEFAULT"\0"

#define BOOTCOMMAND_FSBOOT_DUAL "run ubntboot"
#define CONFIG_EXTRA_ENV_SETTINGS     EXTRA_ENV_SETTINGS_FSBOOT_DUAL

#define	BOOTARGS_SQUASHFS       "console=tty0 root=31:03 rootfstype=squashfs init=/init"
#define BOOTARGS_FSBOOT_DUAL    "console=tty0"
#define	CONFIG_BOOTARGS     BOOTARGS_FSBOOT_DUAL

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
#define CFG_ENV_SIZE		0x10000
#endif

#define CFG_OEM_LEN    CFG_FLASH_SECTOR_SIZE

#ifdef CONFIG_ATH_NAND_FL
#define CONFIG_BOOTCOMMAND "bootm 0x80000"
#define CFG_ENV_ADDR		(0x00040000)
#else

#ifdef ASDK_SPI_BOOTROM_MODE
#define CFG_OEM_ADDR		(0xb9040000)
#define CFG_ENV_ADDR		(0xb9040000)
#define BOOTCOMMAND_SUQASHFS "bootm 0xb9050000"
#define BOOTCOMMAND_FSBOOT_DUAL "run ubntboot"
#define CONFIG_BOOTCOMMAND BOOTCOMMAND_FSBOOT_DUAL

#else
#define CFG_ENV_ADDR		(0xbf040000)
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

#define CONFIG_IPADDR   192.168.1.20
#define CONFIG_SERVERIP 192.168.1.254
#define CONFIG_ETHADDR 0x00:0xaa:0xbb:0xcc:0xdd:0xee
#define CFG_FAULT_ECHO_LINK_DOWN    1


#define WLANCAL_SIZE                    (16 * 1024)
#define WLANCAL(x)                      (0xb9ff1000 + (WLANCAL_SIZE * x))
#define WLANCAL_OFFSET					0x1000
#define BOARDCAL                        0xb9ff0000
#define CAL_SECTOR                      (0xff0)

#define ATHEROS_PRODUCT_ID             137

#undef CONFIG_BOOTDELAY
#define CONFIG_BOOTDELAY    1
#define CFG_CONSOLE_INFO_QUIET 1

#undef CFG_HZ
#define CFG_HZ          (400000000/2)

/* Ubiquiti feature */
#define RECOVERY_FWVERSION_ARCH             "qca851x"
#define AUTO_RESTORE_DEFAULT_ENVIRONMENT    1
#define UBNT_CFG_PART_SIZE                  0x40000 /* 256k(cfg) */
#define UBNT_FSBOOT_DUAL
#define CFG_BOOTM_LEN	(16 << 20) /* 16 MB */
#define DEBUG
#define CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2 "hush>"
#define UNIFI_DETECT_RESET_BTN

#include <cmd_confdefs.h>

#endif	/* __MUSIC_USW_SW24P_H */
