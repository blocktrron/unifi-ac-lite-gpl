/*
 * Copyright (c) 2013 Qualcomm Atheros, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __BOARD_956X_H
#define __BOARD_956X_H

//#include <config.h>

#undef MTDPARTS_DEFAULT

#undef CFG_HZ

#include <atheros.h>


/* Ubiquiti */
#define UNIFI_DETECT_RESET_BTN
#define UBNT_FLASH_DETECT
#define RECOVERY_FWVERSION_ARCH             "qca956x"
#define UBNT_CFG_PART_SIZE                  0x80000 /* 512k(cfg) */

//#include <cmd_confdefs.h>

/* Application code locations
*/

#define UBNT_APP_MAGIC_OFFSET           0x80200000
#define UBNT_APP_START_OFFSET           0x80200020
#define UBNT_APP_SIZE                   (128 * 1024)

#define QCA_11AC 1

/* all layouts we suppported */

#define MTDPARTS_DEVICE  "ath-nor0"
#define MTDIDS_DEFAULT "nor0="MTDPARTS_DEVICE
#define MTDPARTS_INITRAMFS_SIGNED_8	"mtdparts="MTDPARTS_DEVICE":384k(u-boot),64k(u-boot-env),7424k(kernel0),256k(cfg),64k(EEPROM)"
#define MTDPARTS_INITRAMFS_SIGNED_16    "mtdparts="MTDPARTS_DEVICE":384k(u-boot),64k(u-boot-env),7744k(kernel0),7744k(kernel1),128k(bs),256k(cfg),64k(EEPROM)"
#define MTDPARTS_INITRAMFS_SIGNED_32    "mtdparts="MTDPARTS_DEVICE":512k(u-boot),64k(u-boot-env),15104k(kernel0),128k(bs),512k(cfg),64k(EEPROM),15104k(kernel1),1280k(stats)"

/* layout for this board */
#define MTDPARTS_8M_DEFAULT    MTDPARTS_INITRAMFS_SIGNED_8
#define MTDPARTS_16M_DEFAULT   MTDPARTS_INITRAMFS_SIGNED_16
#define MTDPARTS_32M_DEFAULT   MTDPARTS_INITRAMFS_SIGNED_32
#define MTDPARTS_DEFAULT       MTDPARTS_8M_DEFAULT

/* all extra env settings */
#define EXTRA_8M_ENV_SETTINGS_COMMON           "mtdparts="MTDPARTS_8M_DEFAULT"\0\0"
#define EXTRA_16M_ENV_SETTINGS_COMMON           "mtdparts="MTDPARTS_16M_DEFAULT"\0\0"
#define EXTRA_32M_ENV_SETTINGS_COMMON           "mtdparts="MTDPARTS_32M_DEFAULT"\0\0"

#define FIX_MTDPARTS MTDPARTS_INITRAMFS_SIGNED_32

#define KERNEL_FLASH_ADDR_8MB            0x9f070000
#define KERNEL0_FLASH_ADDR_16MB          0x9f070000
#define KERNEL1_FLASH_ADDR_16MB          0x9f800000
#define KERNEL0_FLASH_ADDR_32MB          0x9f090000
#define KERNEL1_FLASH_ADDR_32MB          0x81000000

/* extra env setting for this board */
#define CONFIG_EXTRA_8M_ENV_SETTINGS         EXTRA_8M_ENV_SETTINGS_COMMON
#define CONFIG_EXTRA_16M_ENV_SETTINGS     EXTRA_16M_ENV_SETTINGS_COMMON
#define CONFIG_EXTRA_32M_ENV_SETTINGS     EXTRA_32M_ENV_SETTINGS_COMMON

#define AUTO_RESTORE_DEFAULT_ENVIRONMENT

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

/* all default bootargs */
/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
/* all layouts we suppported for 32MB flash */
#define KILOBYTES_SHIFT             10
#define UBNT_PART_BOOT_OFF          0x0

#define UBNT_PART_BOOT_SIZE         512
#define UBNT_PART_ENV_SIZE          64
#define UBNT_PART_KERNEL_SIZE       15104
#define UBNT_PART_BOOTSEL_SIZE      128
#define UBNT_PART_CFG_SIZE          512
#define UBNT_PART_EEPROM_SIZE       64
#define UBNT_PART_LINUX_KERN1_SIZE  16384  /* 7776 + 64, check why +64 later */

#define UBNT_PART_BOOT_SIZE_H       (UBNT_PART_BOOT_SIZE    << KILOBYTES_SHIFT)
#define UBNT_PART_ENV_SIZE_H        (UBNT_PART_ENV_SIZE     << KILOBYTES_SHIFT)
#define UBNT_PART_KERNEL_SIZE_H     (UBNT_PART_KERNEL_SIZE  << KILOBYTES_SHIFT)
#define UBNT_PART_KERNEL1_SIZE_H    (UBNT_PART_KERNEL_SIZE  << KILOBYTES_SHIFT)
#define UBNT_PART_BOOTSEL_SIZE_H    (UBNT_PART_BOOTSEL_SIZE << KILOBYTES_SHIFT)
#define UBNT_PART_CFG_SIZE_H        (UBNT_PART_CFG_SIZE     << KILOBYTES_SHIFT)
#define UBNT_PART_EEPROM_SIZE_H     (UBNT_PART_EEPROM_SIZE  << KILOBYTES_SHIFT)
#define UBNT_PART_LINUX_KERN1_SIZE_H (UBNT_PART_LINUX_KERN1_SIZE << KILOBYTES_SHIFT)

#define UBNT_PART_ENV_OFF           (UBNT_PART_BOOT_OFF + UBNT_PART_BOOT_SIZE_H)
#define UBNT_PART_KERNEL0_OFF       (UBNT_PART_ENV_OFF + UBNT_PART_ENV_SIZE_H)
#define UBNT_PART_BOOTSEL_OFF       (UBNT_PART_KERNEL0_OFF + UBNT_PART_KERNEL_SIZE_H)
#define UBNT_PART_CFG_OFF           (UBNT_PART_BOOTSEL_OFF + UBNT_PART_BOOTSEL_SIZE_H)
#define UBNT_PART_EEPROM_OFF        (UBNT_PART_CFG_OFF + UBNT_PART_CFG_SIZE_H)
#define UBNT_PART_KERNEL1_OFF       (UBNT_PART_KERNEL0_OFF + UBNT_PART_KERNEL_SIZE_H)  /* TODO: Check Later */


#define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks */
#if (FLASH_SIZE == 32)
#define CFG_MAX_FLASH_SECT  512 /* max number of sectors on one chip */
#define ATH_MTDPARTS_MIB0   "64k(mib0)"
#elif (FLASH_SIZE == 16)
#define CFG_MAX_FLASH_SECT	256	/* max number of sectors on one chip */
#define ATH_MTDPARTS_MIB0	"8256k(mib0)"
#elif (FLASH_SIZE == 8)
#define CFG_MAX_FLASH_SECT	128	/* max number of sectors on one chip */
#define ATH_MTDPARTS_MIB0	"64k(mib0)"
#else
#define CFG_MAX_FLASH_SECT	64	/* max number of sectors on one chip */
#define ATH_MTDPARTS_MIB0	"64k(mib0)"
#endif

#define CFG_FLASH_SECTOR_SIZE	(64*1024)
#if (FLASH_SIZE == 32)
#define CFG_FLASH_SIZE      0x02000000  /* Total flash size */
#elif (FLASH_SIZE == 16)
#define CFG_FLASH_SIZE		0x01000000	/* Total flash size */
#elif (FLASH_SIZE == 8)
#define CFG_FLASH_SIZE		0x00800000	/* max number of sectors on one chip */
#else
#define CFG_FLASH_SIZE		0x00400000	/* Total flash size */
#endif

#ifndef COMPRESSED_UBOOT
#define ENABLE_DYNAMIC_CONF	1
#endif

#undef CFG_ATHRS26_PHY

#if (CFG_MAX_FLASH_SECT * CFG_FLASH_SECTOR_SIZE) != CFG_FLASH_SIZE
#	error "Invalid flash configuration"
#endif

#define CFG_FLASH_WORD_SIZE	unsigned short

#if defined(CONFIG_ATH_NAND_BR) && defined(COMPRESSED_UBOOT)
#define CFG_FLASH_BASE			0xa0100000
#else
/* NOR Flash start address */
#define CFG_FLASH_BASE			0x9f000000
#endif

#ifdef COMPRESSED_UBOOT
#define BOOTSTRAP_TEXT_BASE		CFG_FLASH_BASE
#define BOOTSTRAP_CFG_MONITOR_BASE	BOOTSTRAP_TEXT_BASE
#endif

#define CONFIG_PCI_CONFIG_DATA_IN_OTP

/*
 * Defines to change flash size on reboot
 */
#ifdef ENABLE_DYNAMIC_CONF
#define UBOOT_FLASH_SIZE		(512 * 1024)
#define UBOOT_ENV_SEC_START		(CFG_FLASH_BASE + UBOOT_FLASH_SIZE)

#define CFG_FLASH_MAGIC			0xaabacada
#define CFG_FLASH_MAGIC_F		(UBOOT_ENV_SEC_START + CFG_FLASH_SECTOR_SIZE - 0x20)
#define CFG_FLASH_SECTOR_SIZE_F		*(volatile int *)(CFG_FLASH_MAGIC_F + 0x4)
#define CFG_FLASH_SIZE_F		*(volatile int *)(CFG_FLASH_MAGIC_F + 0x8) /* Total flash size */
#define CFG_MAX_FLASH_SECT_F		(CFG_FLASH_SIZE / CFG_FLASH_SECTOR_SIZE) /* max number of sectors on one chip */
#else
#define CFG_FLASH_SIZE_F		CFG_FLASH_SIZE
#define CFG_FLASH_SECTOR_SIZE_F		CFG_FLASH_SECTOR_SIZE
#endif


// DDR2
// 0x40c3   25MHz
// 0x4138   40MHz
// DDR1
// 0x4186   25Mhz
// 0x4270   40Mhz

#define CFG_DDR_REFRESH_VAL		0x4186
#define CFG_DDR2_REFRESH_VAL    0x40c3
/*
 * The following #defines are needed to get flash environment right
 */
#define	CFG_MONITOR_BASE	TEXT_BASE
#define	CFG_MONITOR_LEN		(192 << 10)

#define __gen_cmd(n, a, f, ec, cc, el)		\
	#n "=tftp 0x80060000 ${dir}" #f "&&"	\
	#ec " " #a " " #el "&&"			\
	#cc " $fileaddr " #a " $filesize\0"

#define gen_cmd(n, a, f)			\
	__gen_cmd(n, a, f, erase, cp.b, +$filesize)

#define gen_cmd_el(n, a, f, el)			\
	__gen_cmd(n, a, f, erase, cp.b, +el)

#define nand_gen_cmd(n, a, f, s)		\
	__gen_cmd(n, a, f, nand erase, nand write, s)

#define __fs_name(x, y)		x ## y
#define _fs_name(x, y)		__fs_name(x, y)
#define fs_name(y)		_fs_name(__CONFIG_BOARD_NAME, y)

#ifdef COMPRESSED_UBOOT
#	define ATH_U_FILE	tuboot.bin
#else
#	define ATH_U_FILE	u-boot.bin
#endif


#if 0
#ifdef CONFIG_ATH_NAND_SUPPORT
#	ifdef CONFIG_ATH_NAND_BR	// nand boot rom
#		if defined(COMPRESSED_UBOOT)
#			define ATH_U_CMD	nand_gen_cmd(lu, 0x0, 2fw.bin, 0x20000)
#			define MTDPARTS_DEFAULT	"mtdparts=ath-nand:128k(u-boot),384k(free),1280k(uImage),7m(rootfs),128k(dummy),128k(caldata)"
#		else
#		define ATH_U_CMD	nand_gen_cmd(lu, 0x0, 2fw.bin, 0x40000)
#		define MTDPARTS_DEFAULT "mtdparts=ath-nand:256k(u-boot),256k(u-boot-env),1280k(uImage),7m(rootfs),128k(dummy),128k(caldata)"
#		endif
#		define ATH_ROOT_DEV	"31:03"
#		define CFG_ENV_ADDR	0x00040000
#	else //dual flash
#		define ATH_U_CMD	gen_cmd(lu, 0x9f000000, ATH_U_FILE)
#		define MTDPARTS_DEFAULT "mtdparts=ath-nor0:320k(u-boot-and-env);ath-nand:512k(pad),1280k(uImage),7m(rootfs),128k(dummy),128k(caldata)"
#		define ATH_ROOT_DEV	"31:03"
#		define CFG_ENV_ADDR	0x9f040000
#	endif
#	define ATH_F_FILE		fs_name(${bc}-nand-jffs2)
#	define ATH_F_LEN		0x700000
#	define ATH_F_ADDR		0x1c0000
#	define ATH_K_FILE		vmlinux${bc}.lzma.uImage
#	define ATH_K_ADDR		0x80000
#	define ATH_F_CMD		nand_gen_cmd(lf, ATH_F_ADDR, ATH_F_FILE, ATH_F_LEN)
#	define ATH_K_CMD		nand_gen_cmd(lk, ATH_K_ADDR, ATH_K_FILE, 0x140000)
#	define ATH_EXTRA_ENV		"bootdevice=0\0"
#else
#	define ATH_F_FILE		fs_name(${bc}-jffs2)
#	define ATH_F_LEN		0x630000
#	define ATH_F_ADDR		0x9f050000
#	define ATH_K_FILE		vmlinux${bc}.lzma.uImage
#	define ATH_K_ADDR		0x9f680000
#	define MTDPARTS_DEFAULT		"mtdparts=ath-nor0:384k(u-boot),64k(u-boot-env),6336k(rootfs),1280k(uImage)," ATH_MTDPARTS_MIB0 ",64k(ART)"
#endif /*CONFIG_MI124*/
#endif

#ifndef ATH_ROOT_DEV
#	define ATH_ROOT_DEV	"31:02"
#endif

#ifndef ATH_EXTRA_ENV
#	define ATH_EXTRA_ENV
#endif

#ifndef ATH_U_CMD
#	define ATH_U_CMD	gen_cmd(lu, 0x9f000000, ATH_U_FILE)
#endif

#ifndef ATH_F_CMD
#	define ATH_F_CMD	gen_cmd_el(lf, ATH_F_ADDR, ATH_F_FILE, ATH_F_LEN)
#endif

#ifndef ATH_K_CMD
#	define ATH_K_CMD	gen_cmd(lk, ATH_K_ADDR, ATH_K_FILE)
#endif

#if 0
#define CONFIG_EXTRA_8M_ENV_SETTINGS	\
	"dir=\0" ATH_U_CMD ATH_F_CMD ATH_K_CMD ""

#define CONFIG_EXTRA_16M_ENV_SETTINGS	\
	"dir=\0" ATH_U_CMD ATH_F_CMD ATH_K_CMD ""

#define	CONFIG_BOOTARGS		"console=ttyS0,115200 root=" ATH_ROOT_DEV " rootfstype=jffs2 init=/sbin/init " MTDPARTS_DEFAULT
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
#define CFG_INIT_SRAM_SP_OFFSET	0xbd001800

#ifdef CONFIG_ATH_NAND_SUPPORT
#	define CONFIG_BOOTCOMMAND	"nboot 0x81000000 0 0x80000"
#else
#	define CFG_ENV_ADDR		0x9f080000
#	define CONFIG_BOOTCOMMAND	"run ubntappinit; go $ubntaddr ubntboot;bootm $flash_boot_addr"
#endif


#ifdef ENABLE_DYNAMIC_CONF
#define CFG_DDR_MAGIC		0xaabacada
#define CFG_DDR_MAGIC_F		(UBOOT_ENV_SEC_START + CFG_FLASH_SECTOR_SIZE - 0x30)
#define CFG_DDR_CONFIG_VAL_F	*(volatile int *)(CFG_DDR_MAGIC_F + 4)
#define CFG_DDR_CONFIG2_VAL_F	*(volatile int *)(CFG_DDR_MAGIC_F + 8)
#define CFG_DDR_EXT_MODE_VAL_F	*(volatile int *)(CFG_DDR_MAGIC_F + 12)
#endif

#define CONFIG_NET_MULTI
#define CONFIG_MEMSIZE_IN_BYTES
#define CONFIG_PCI 1
#define CONFIG_USB 1


/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#ifndef COMPRESSED_UBOOT
#define ATH_CFG_COMMANDS	((			\
				CONFIG_CMD_DFL	|	\
				CFG_CMD_DHCP	|	\
				CFG_CMD_ELF	|	\
				CFG_CMD_PCI	|	\
				CFG_CMD_FLS	|	\
				CFG_CMD_MII	|	\
				CFG_CMD_LOADB	|	\
				CFG_CMD_LOADS	|	\
				CFG_CMD_PING	|	\
				CFG_CMD_NET	|	\
				CFG_CMD_ENV	|	\
				CFG_CMD_PLL	|	\
				CFG_CMD_FLASH	|	\
				CFG_CMD_RUN	|	\
				CFG_CMD_ELF	|	\
				CFG_CMD_DDR	|	\
				CFG_CMD_ETHREG		\
				) & ~(			\
				CFG_CMD_IMLS	|	\
				CFG_CMD_FLASH		\
				))
#else
#	ifdef CONFIG_ATH_NAND_BR
#		define ATH_CFG_COMMANDS		((			\
						CONFIG_CMD_DFL	|	\
						CFG_CMD_PING	|	\
						CFG_CMD_NET) & ~(	\
						CFG_CMD_FLASH		\
						))
#	else
#		define ATH_CFG_COMMANDS		(CONFIG_CMD_DFL	|	\
				CFG_CMD_PING	|	\
				CFG_CMD_NET | CFG_CMD_JFFS2)
#	endif
#endif /* #ifndef COMPRESSED_UBOOT */

#ifdef CONFIG_ATH_NAND_SUPPORT
#	ifdef CONFIG_ATH_NAND_BR
#		define CFG_ENV_IS_IN_NAND	1
#		define CFG_ENV_OFFSET		0x40000u
#		define CFG_ENV_SIZE		0x40000u
#		define ATH_EXTRA_CMD		CFG_CMD_NAND
#	else
#		define CFG_ENV_IS_IN_FLASH	1
#		define CFG_ENV_SIZE		CFG_FLASH_SECTOR_SIZE
#		define ATH_EXTRA_CMD		(CFG_CMD_NAND | CFG_CMD_FLASH)
#	endif
#	define NAND_MAX_CHIPS			1
#	define CFG_MAX_NAND_DEVICE		1
#else
#	define ATH_EXTRA_CMD			CFG_CMD_FLASH
#	define CFG_ENV_IS_IN_FLASH		1
#	define CFG_ENV_SIZE			CFG_FLASH_SECTOR_SIZE
#endif

#define DEBUG

#ifdef COMPRESSED_UBOOT
#undef  CFG_ENV_IS_IN_FLASH
#undef  CFG_ENV_IS_IN_NAND
#define CFG_ENV_IS_NOWHERE		1
#endif

#define CONFIG_COMMANDS	(( CFG_CMD_MEMORY | CFG_CMD_DHCP | CFG_CMD_JFFS2 | CFG_CMD_PCI | \
	CFG_CMD_MII | CFG_CMD_LOADB | CFG_CMD_LOADS  | CFG_CMD_PING | CFG_CMD_NET | CFG_CMD_ENV | CFG_CMD_PLL| \
	CFG_CMD_FLASH | CFG_CMD_RUN | CFG_CMD_TFTP_SERVER | CFG_CMD_BOOTD ))


//#define CONFIG_COMMANDS                 (ATH_CFG_COMMANDS | ATH_EXTRA_CMD | CFG_CMD_JFFS2)

#define CONFIG_IPADDR			192.168.1.20
#define CONFIG_SERVERIP			192.168.1.254
#define CONFIG_ETHADDR			0x00:0xaa:0xbb:0xcc:0xdd:0xee
#define CFG_FAULT_ECHO_LINK_DOWN	1

#define CFG_PHY_ADDR			0
#define CFG_GMII			0
#define CFG_MII0_RMII			1
#define CFG_AG7100_GE0_RMII		1

#define CFG_BOOTM_LEN			(16 << 20) /* 16 MB */
#define CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2		"hush>"

/*
** Parameters defining the location of the calibration/initialization
** information for the two Merlin devices.
** NOTE: **This will change with different flash configurations**
*/

//#define CAL_ON_LAST_SECTOR
#define WLANCAL_SIZE                    (16 * 1024)
#ifdef CAL_ON_LAST_SECTOR
#define WLANCAL(x)                      (ar724x_get_wlancfg_addr(x))
#define BOARDCAL                (ar724x_get_bdcfg_addr())
#define CAL_SECTOR              (ar724x_get_bdcfg_sector())
#define WLANCAL_OFFSET                                  0x1000
#define BOARDCAL_OFFSET                                 0x0
#else
#define WLANCAL(x)                      (0x9fff1000 + (WLANCAL_SIZE * x))
#define BOARDCAL                        0x9fff0000
#define CAL_SECTOR                      (CFG_MAX_FLASH_SECT - 1)
#define WLANCAL_OFFSET                                  0x1000
#define BOARDCAL_OFFSET                                 0x0
#endif

#define ATHEROS_PRODUCT_ID		137

/* For Merlin, both PCI, PCI-E interfaces are valid */
#define ATH_ART_PCICFG_OFFSET		12

#include <cmd_confdefs.h>

#endif	/* __BOARD_956X_H */
