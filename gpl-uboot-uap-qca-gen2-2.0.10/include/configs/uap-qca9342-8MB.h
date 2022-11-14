/*
 * This file contains the configuration parameters for the pb93 board.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <configs/ar7240.h>

#define FLASH_SIZE 8
/*-----------------------------------------------------------------------
 * FLASH and environment organization
 * Set CFG_MAX_FLASH_SECT to the max number of sectors u-boot support.
 * If we don't do that we will be in trouble writing sectors.
 * Because size of some arrays are declared statically by this number.
 * ex: start & protect in include/flash.h
 */
#define CFG_MAX_FLASH_BANKS     1	    /* max number of memory banks */
#if (FLASH_SIZE == 16)
#define CFG_MAX_FLASH_SECT      256    /* max number of sectors on one chip */
#elif (FLASH_SIZE == 8)
#define CFG_MAX_FLASH_SECT      128    /* max number of sectors on one chip */
#else
#define CFG_MAX_FLASH_SECT      64    /* max number of sectors on one chip */
#endif

#define CFG_FLASH_SECTOR_SIZE   (64*1024)
#if (FLASH_SIZE == 16)
#define CFG_FLASH_SIZE          0x01000000 /* Total flash size */
#elif (FLASH_SIZE == 8)
#define CFG_FLASH_SIZE          0x00800000    /* max number of sectors on one chip */
#else
#define CFG_FLASH_SIZE          0x00400000 /* Total flash size */
#endif

#define ENABLE_DYNAMIC_CONF 1

#define CONFIG_WASP_SUPPORT 1
#define CFG_INIT_SRAM_SP_OFFSET 0xbd007000
#undef CFG_ATHRS26_PHY

#if (CFG_MAX_FLASH_SECT * CFG_FLASH_SECTOR_SIZE) != CFG_FLASH_SIZE
#	error "Invalid flash configuration"
#endif

#define CFG_FLASH_WORD_SIZE     unsigned short

// Set this to zero, for 16bit ddr2
#define DDR2_32BIT_SUPPORT 	1

/* S16 specific defines */

#define CONFIG_AG7240_GE0_IS_CONNECTED 1

/* Assuming nand flash wouldn't use compressed u-boot */
#ifdef CONFIG_ATH_NAND_FL
#	define CFG_FLASH_BASE			0
#else
	/*
	 * We boot from this flash
	 */
#	define CFG_FLASH_BASE			0x9f000000
#endif

/* Application code locations
*/

#define UBNT_APP_MAGIC_OFFSET           0x80200000
#define UBNT_APP_START_OFFSET           0x80200020
#define UBNT_APP_SIZE                   (128 * 1024)

/*
 * Defines to change flash size on reboot
 */
#ifdef ENABLE_DYNAMIC_CONF
#define UBOOT_FLASH_SIZE          (384 * 1024)
#define UBOOT_ENV_SEC_START        (CFG_FLASH_BASE + UBOOT_FLASH_SIZE)

#define CFG_FLASH_MAGIC           0xaabacada
#define CFG_FLASH_MAGIC_F         (UBOOT_ENV_SEC_START + CFG_FLASH_SECTOR_SIZE - 0x20)
#define CFG_FLASH_SECTOR_SIZE_F   *(volatile int *)(CFG_FLASH_MAGIC_F + 0x4)
#define CFG_FLASH_SIZE_F          *(volatile int *)(CFG_FLASH_MAGIC_F + 0x8) /* Total flash size */
#define CFG_MAX_FLASH_SECT_F      (CFG_FLASH_SIZE / CFG_FLASH_SECTOR_SIZE) /* max number of sectors on one chip */
#else
#define CFG_FLASH_SIZE_F          CFG_FLASH_SIZE
#define CFG_FLASH_SECTOR_SIZE_F   CFG_FLASH_SECTOR_SIZE
#endif

/*
 * The following #defines are needed to get flash environment right
 */
#define	CFG_MONITOR_BASE	TEXT_BASE
#define	CFG_MONITOR_LEN		(192 << 10)

/* all layouts we suppported */
#define MTDPARTS_DEVICE  "ath-nor0"
#define MTDIDS_DEFAULT "nor0="MTDPARTS_DEVICE
#define MTDPARTS_SQUASHFS_8	"mtdparts="MTDPARTS_DEVICE":256k(u-boot),64k(u-boot-env),1024k(kernel),6528k(rootfs),256k(cfg),64k(EEPROM)"
#define MTDPARTS_INITRAMFS_SIGNED_8	"mtdparts="MTDPARTS_DEVICE":384k(u-boot),64k(u-boot-env),7424k(kernel),256k(cfg),64k(EEPROM)"
#define MTDPARTS_FSBOOT_DUAL_8 "mtdparts="MTDPARTS_DEVICE":256k(u-boot),64k(u-boot-env),7552k(jffs2),256k(cfg),64k(EEPROM)"
#define MTDPARTS_FSBOOT_DUAL_16 "mtdparts="MTDPARTS_DEVICE":256k(u-boot),64k(u-boot-env),15744k(jffs2),256k(cfg),64k(EEPROM)"

/* layout for this board */
#define MTDPARTS_DEFAULT    MTDPARTS_INITRAMFS_SIGNED_8

/* all extra env settings */
#define EXTRA_ENV_SETTINGS_COMMON           "mtdparts="MTDPARTS_DEFAULT"\0"
#define EXTRA_ENV_SETTINGS_RAW              EXTRA_ENV_SETTINGS_COMMON   \
            "ubntboot=bootm 0x9f050000\0"
#define EXTRA_ENV_SETTINGS_RAW_SIGNED       EXTRA_ENV_SETTINGS_COMMON   \
            "ubntboot=bootm 0x9f070000\0"
#define EXTRA_ENV_SETTINGS_FSBOOT_DUAL      EXTRA_ENV_SETTINGS_COMMON   \
            "ubntbootaddr=0x81000000\0"		\
            "ubntboot=ubntfsboot ${ubntbootaddr} jffs2 kernel\0"

/* extra env setting for this board */
#define CONFIG_EXTRA_ENV_SETTINGS     EXTRA_ENV_SETTINGS_RAW_SIGNED

/* all default bootargs */
#define BOOTARGS_DEFAULT_PANIC_TIMEOUT  "3"
#define BOOTARGS_DEFAULT_CONSOLE        "tty0"
#define BOOTARGS_DEFAULT_COMMON         "console="BOOTARGS_DEFAULT_CONSOLE" panic="BOOTARGS_DEFAULT_PANIC_TIMEOUT
#define	BOOTARGS_DEFAULT_SQUASHFS       "root=31:03 rootfstype=squashfs init=/init "BOOTARGS_DEFAULT_COMMON
#define BOOTARGS_DEFAULT_INITRAMFS      BOOTARGS_DEFAULT_COMMON

/* default bootargs for this board */
#define	CONFIG_BOOTARGS     BOOTARGS_DEFAULT_INITRAMFS

#define EXTRA_UBNTAPP_SETTINGS \
            "ubntappinit=go ${ubntaddr} uappinit;go ${ubntaddr} ureset_button;urescue;go ${ubntaddr} uwrite\0"

#undef CFG_PLL_FREQ

//#define CFG_PLL_FREQ	CFG_PLL_600_266_133
//#define CFG_PLL_FREQ	CFG_PLL_600_525_262
//#define CFG_PLL_FREQ	CFG_PLL_566_550_275
//#define CFG_PLL_FREQ	CFG_PLL_566_525_262
//#define CFG_PLL_FREQ	CFG_PLL_600_332_166
//#define CFG_PLL_FREQ	CFG_PLL_566_475_237
//#define CFG_PLL_FREQ	CFG_PLL_600_575_287
//#define CFG_PLL_FREQ	CFG_PLL_400_400_200
//#define CFG_PLL_FREQ	CFG_PLL_533_400_200
//#define CFG_PLL_FREQ	CFG_PLL_600_450_200
//#define CFG_PLL_FREQ	CFG_PLL_600_500_1G_250
//#define CFG_PLL_FREQ	CFG_PLL_600_550_1_1G_275
//#define CFG_PLL_FREQ	CFG_PLL_600_350_175
//#define CFG_PLL_FREQ	CFG_PLL_600_300_150
//#define CFG_PLL_FREQ	CFG_PLL_600_1_2G_400_200
//#define CFG_PLL_FREQ	CFG_PLL_300_300_150
//#define CFG_PLL_FREQ	CFG_PLL_500_400_200
//#define CFG_PLL_FREQ	CFG_PLL_400_200_200
#define CFG_PLL_FREQ    CFG_PLL_560_450_225
///#define CFG_PLL_FREQ	CFG_PLL_535_400_200

#undef CFG_HZ
/*
 * MIPS32 24K Processor Core Family Software User's Manual
 *
 * 6.2.9 Count Register (CP0 Register 9, Select 0)
 * The Count register acts as a timer, incrementing at a constant
 * rate, whether or not an instruction is executed, retired, or
 * any forward progress is made through the pipeline.  The counter
 * increments every other clock, if the DC bit in the Cause register
 * is 0.
 */
/* Since the count is incremented every other tick, divide by 2 */
/* XXX derive this from CFG_PLL_FREQ */
#if (CFG_PLL_FREQ == CFG_PLL_200_200_100)
#   define CFG_HZ          (200000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_300_300_150)
#   define CFG_HZ          (300000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_350_350_175)
#   define CFG_HZ          (350000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_333_333_166)
#   define CFG_HZ          (333000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_266_266_133)
#   define CFG_HZ          (266000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_266_266_66)
#   define CFG_HZ          (266000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_400_400_200) || (CFG_PLL_FREQ == CFG_PLL_400_400_100)
#   define CFG_HZ          (400000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_320_320_80) || (CFG_PLL_FREQ == CFG_PLL_320_320_160)
#   define CFG_HZ          (320000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_410_400_200)
#   define CFG_HZ          (410000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_420_400_200)
#   define CFG_HZ          (420000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_240_240_120)
#   define CFG_HZ          (240000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_160_160_80)
#   define CFG_HZ          (160000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_400_200_200) || (CFG_PLL_FREQ == CFG_PLL_400_480_240)
#   define CFG_HZ          (400000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_500_400_200)
#   define CFG_HZ          (500000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_600_400_200) || (CFG_PLL_FREQ == CFG_PLL_600_450_200) || \
      (CFG_PLL_FREQ == CFG_PLL_600_600_300) || (CFG_PLL_FREQ == CFG_PLL_600_550_275) || \
      (CFG_PLL_FREQ == CFG_PLL_600_332_166) || (CFG_PLL_FREQ == CFG_PLL_600_575_287) || \
      (CFG_PLL_FREQ == CFG_PLL_600_525_262) || (CFG_PLL_FREQ == CFG_PLL_600_332_200) || \
      (CFG_PLL_FREQ == CFG_PLL_600_266_133) || (CFG_PLL_FREQ == CFG_PLL_600_266_200) || \
      (CFG_PLL_FREQ == CFG_PLL_600_650_325) ||  (CFG_PLL_FREQ == CFG_PLL_600_1_2G_400_200)
#   define CFG_HZ          (600000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_600_1_2G_400_200)
#   define CFG_HZ          (600000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_600_500_1G_250)
#   define CFG_HZ          (600000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_600_550_1_1G_275)
#   define CFG_HZ          (600000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_600_500_250)
#   define CFG_HZ          (600000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_600_350_175)
#   define CFG_HZ          (600000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_600_300_150)
#   define CFG_HZ          (600000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_533_400_200) || (CFG_PLL_FREQ == CFG_PLL_533_500_250)
#   define CFG_HZ          (533000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_535_400_200)
#   define CFG_HZ          (535000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_700_400_200)
#   define CFG_HZ          (700000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_650_600_300)
#   define CFG_HZ          (650000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_560_480_240) || (CFG_PLL_FREQ == CFG_PLL_560_400_200) || \
      (CFG_PLL_FREQ == CFG_PLL_560_450_225)
#   define CFG_HZ          (560000000/2)
#elif (CFG_PLL_FREQ == CFG_PLL_566_475_237) || (CFG_PLL_FREQ == CFG_PLL_566_450_225) || \
      (CFG_PLL_FREQ == CFG_PLL_566_550_275) || (CFG_PLL_FREQ == CFG_PLL_566_525_262) || \
      (CFG_PLL_FREQ == CFG_PLL_566_400_200) || (CFG_PLL_FREQ == CFG_PLL_566_500_250)
#   define CFG_HZ          (566000000/2)
#endif

/*
 * timeout values are in ticks
 */
#define CFG_FLASH_ERASE_TOUT	(2 * CFG_HZ) /* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT	(2 * CFG_HZ) /* Timeout for Flash Write */

/*
 * Cache lock for stack
 */
#define CFG_INIT_SP_OFFSET	0x1000

#define CFG_ENV_IS_IN_FLASH 1
#undef CFG_ENV_IS_NOWHERE

/* Address and size of Primary Environment Sector	*/
#define CFG_ENV_SIZE		CFG_FLASH_SECTOR_SIZE

#define UBNT_ENV_ADDR           0x9f040000
#define UBNT_ENV_ADDR_SIGNED    0x9f060000
#define CFG_ENV_ADDR		    UBNT_ENV_ADDR_SIGNED

#define BOOTCOMMAND_RAW         "bootm 0x9f050000"
#define BOOTCOMMAND_RAW_SIGNED  "bootm 0x9f070000"
#define BOOTCOMMAND_FSBOOT_DUAL "run ubntboot"
#define BOOTCOMMAND_UBNTAPP     "run ubntappinit ubntboot"
#define CONFIG_BOOTCOMMAND      BOOTCOMMAND_UBNTAPP

//#define CONFIG_FLASH_16BIT

/* DDR init values */

#define CONFIG_NR_DRAM_BANKS	2

/* DDR settings for WASP */

#define CFG_DDR_REFRESH_VAL     0x4270
#define CFG_DDR_CONFIG_VAL      0xc7bc8cd0
#define CFG_DDR_MODE_VAL_INIT   0x133
#define CFG_DDR_EXT_MODE_VAL    0x0
#define CFG_DDR_MODE_VAL        0x33

#define CFG_DDR_TRTW_VAL        0x1f
#define CFG_DDR_TWTR_VAL        0x1e

#define CFG_DDR_CONFIG2_VAL     0x9dd0e6a8



#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_32	0xff
#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_16	0xffff

#if DDR2_32BIT_SUPPORT
#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL		CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_32
#else
#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL		CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_16
#endif

#define CFG_DDR1_RD_DATA_THIS_CYCLE_VAL		0xffff
#define CFG_SDRAM_RD_DATA_THIS_CYCLE_VAL	0xffffffff

/* DDR2 Init values */
#define CFG_DDR2_EXT_MODE_VAL    0x402


#ifdef ENABLE_DYNAMIC_CONF
#define CFG_DDR_MAGIC           0xaabacada
#define CFG_DDR_MAGIC_F         (UBOOT_ENV_SEC_START + CFG_FLASH_SECTOR_SIZE - 0x30)
#define CFG_DDR_CONFIG_VAL_F    *(volatile int *)(CFG_DDR_MAGIC_F + 4)
#define CFG_DDR_CONFIG2_VAL_F	*(volatile int *)(CFG_DDR_MAGIC_F + 8)
#define CFG_DDR_EXT_MODE_VAL_F  *(volatile int *)(CFG_DDR_MAGIC_F + 12)
#endif

#define CONFIG_NET_MULTI
#define CONFIG_MEMSIZE_IN_BYTES
#define CONFIG_PCI 1

/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CONFIG_COMMANDS	(( CFG_CMD_MEMORY | CFG_CMD_DHCP | CFG_CMD_JFFS2 | CFG_CMD_PCI | \
	CFG_CMD_MII | CFG_CMD_PING | CFG_CMD_NET | CFG_CMD_ENV | CFG_CMD_PLL| \
	CFG_CMD_FLASH | CFG_CMD_RUN | CFG_CMD_TFTP_SERVER | CFG_CMD_BOOTD ))

#define CONFIG_IPADDR   192.168.1.20
#define CONFIG_SERVERIP 192.168.1.254
#define CONFIG_ETHADDR 0x00:0xaa:0xbb:0xcc:0xdd:0xee
#define CFG_FAULT_ECHO_LINK_DOWN    1


#define CFG_PHY_ADDR 0
#define CFG_GMII     0
#define CFG_MII0_RMII             1
#define CFG_AG7100_GE0_RMII             1

#define CFG_BOOTM_LEN	(16 << 20) /* 16 MB */
#if 0
#define DEBUG
#define CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2 "hush>"
#endif

/*
** Parameters defining the location of the calibration/initialization
** information for the two Merlin devices.
** NOTE: **This will change with different flash configurations**
*/

#define CAL_ON_LAST_SECTOR
#define WLANCAL_SIZE                    (16 * 1024)
#ifdef CAL_ON_LAST_SECTOR
#define WLANCAL(x)			(ar724x_get_wlancfg_addr(x))
#define BOARDCAL		(ar724x_get_bdcfg_addr())
#define CAL_SECTOR		(ar724x_get_bdcfg_sector())
#define WLANCAL_OFFSET					0x1000
#define BOARDCAL_OFFSET					0x0
#else
#define WLANCAL(x)                      (0x9fff1000 + (WLANCAL_SIZE * x))
#define BOARDCAL                        0x9fff0000
#define CAL_SECTOR                      (CFG_MAX_FLASH_SECT - 1)
#endif
#define ATHEROS_PRODUCT_ID              137

/* For Merlin, both PCI, PCI-E interfaces are valid */
#define AR7240_ART_PCICFG_OFFSET        12

#undef CONFIG_BOOTDELAY
#define CONFIG_BOOTDELAY    1
#define CFG_CONSOLE_INFO_QUIET 1
#define CONFIG_PCI_CONFIG_DATA_IN_OTP

/* Ubiquiti */
#define UNIFI_DETECT_RESET_BTN
#define UBNT_FLASH_DETECT
#define RECOVERY_FWVERSION_ARCH             "qca9342"
#define CONFIG_JFFS2_CMDLINE
//#define UBNT_FSBOOT_DUAL
#define UBNT_CFG_PART_SIZE                  0x40000 /* 256k(cfg) */

#include <cmd_confdefs.h>

#endif	/* __CONFIG_H */
