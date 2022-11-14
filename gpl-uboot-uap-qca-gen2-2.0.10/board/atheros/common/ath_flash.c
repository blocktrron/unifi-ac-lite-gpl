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

#include <common.h>
#include <jffs2/jffs2.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include <atheros.h>
#include "ath_flash.h"

#if !defined(ATH_DUAL_FLASH)
#	define	ath_spi_flash_print_info	flash_print_info
#endif

#if ENABLE_EXT_ADDR_SUPPORT
/* for legacy flash only support 16M */
#define LEGACY_SPI_ADDR_BOUNDARY 0x1000000
#endif

#ifdef UBNT_FLASH_DETECT
typedef struct spi_flash {
	char* name;
	u32 jedec_id;
	u32 sector_count;
	u32 sector_size;
} spi_flash_t;

#define SECTOR_SIZE_64K                 (64*1024)
#define SECTOR_SIZE_128K                (128*1024)
#define SECTOR_SIZE_256K                (256*1024)

static spi_flash_t SUPPORTED_FLASH_LIST[] = {
	/* JEDEC id's for supported flashes ONLY */
	/* ST Microelectronics */
	{ "m25p64",  0x202017,	128, SECTOR_SIZE_64K },  /*  8MB */
	{ "m25p128", 0x202018,	 64, SECTOR_SIZE_256K }, /* 16MB */

	/* Macronix */
	{ "mx25l64",  0xc22017, 128, SECTOR_SIZE_64K },  /*  8MB */
	{ "mx25l128", 0xc22018, 256, SECTOR_SIZE_64K },  /* 16MB */
	{ "mx25l256", 0xc22019, 512, SECTOR_SIZE_64K },  /* 32MB */

	/* Spansion */
	{ "s25sl064a", 0x010216, 128, SECTOR_SIZE_64K }, /*  8MB */
	{ "s25fl064k", 0x012017, 128, SECTOR_SIZE_64K }, /*  8MB */
	{ "s25fl128k", 0x012018, 256, SECTOR_SIZE_64K }, /*  16MB */

	/* Intel */
	{ "s33_64M",    0x898913, 128, SECTOR_SIZE_64K }, /*  8MB */

	/* Winbond */
	{ "w25x64",	0xef3017, 128, SECTOR_SIZE_64K }, /*  8MB */
	{ "w25q64",	0xef4017, 128, SECTOR_SIZE_64K }, /*  8MB */
	{ "w25q128",	0xef4018, 256, SECTOR_SIZE_64K }, /*  16MB */

    /* Micron, Numonyx */
    { "n25q064", 0x20ba17, 128, SECTOR_SIZE_64K }, /*  8MB */
    { "n25q128", 0x20ba18, 256, SECTOR_SIZE_64K }, /*  16MB */

    /* EON */
    { "en25qh64", 0x1c7017, 128, SECTOR_SIZE_64K }, /*  8MB */
    { "en25qh128", 0x1c7018, 256, SECTOR_SIZE_64K }, /*  16MB */

};

#define N(a) (sizeof((a)) / sizeof((a)[0]))
#endif /* UBNT_FLASH_DETECT */

/*
 * globals
 */
flash_info_t flash_info[CFG_MAX_FLASH_BANKS];

extern void unifi_set_led(int);
static int display_enabled = 0;

int flash_status_display(int enable) {
	display_enabled = enable;
    if (!display_enabled) {
        unifi_set_led(0);
    }
    return 0;
}

static int led_flip = 0;
static u32 led_tick = 0;

static void unifi_toggle_led(u32 bound) {
    if (!display_enabled) return;
    led_tick ++;
    if (led_tick > bound) {
        led_flip = (1 - led_flip);
        unifi_set_led(led_flip + 1);
        led_tick %= bound;
    }

}

/*
 * statics
 */
static void ath_spi_write_enable(void);
static void ath_spi_poll(void);
#if !defined(ATH_SST_FLASH)
static void ath_spi_write_page(uint32_t addr, uint8_t * data, int len);
#endif
static void ath_spi_sector_erase(uint32_t addr);

#if ENABLE_EXT_ADDR_SUPPORT
static void ath_spi_wrear(uint32_t data);
static uchar ath_spi_rdear(void);
#endif

#ifdef UBNT_FLASH_DETECT
static u32
ath_spi_read_id(u8 id_cmd, u8* data, int n)
{
	int i = 0;
	ath_reg_wr_nf(ATH_SPI_FS, 1);
	ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
	ath_spi_bit_banger(id_cmd);
	while (i < n) {
		ath_spi_delay_8();
		data[i] = ath_reg_rd(ATH_SPI_RD_STATUS) & 0xff;
		++i;
	}
	ath_spi_done();
	return 0;
}

static spi_flash_t*
find_flash_data(u32 jedec)
{
	spi_flash_t* tmp;
	int i;
	for (i = 0, tmp = &SUPPORTED_FLASH_LIST[0];
			i < N(SUPPORTED_FLASH_LIST);
			i++, tmp++) {
		if (tmp->jedec_id == jedec)
			return tmp;
	}
	return NULL;
}
#else
static void
ath_spi_read_id(void)
{
	u32 rd;

	ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
	ath_spi_bit_banger(ATH_SPI_CMD_RDID);
	ath_spi_delay_8();
	ath_spi_delay_8();
	ath_spi_delay_8();
	ath_spi_go();

	rd = ath_reg_rd(ATH_SPI_RD_STATUS);

	printf("Flash Manuf Id 0x%x, DeviceId0 0x%x, DeviceId1 0x%x\n",
		(rd >> 16) & 0xff, (rd >> 8) & 0xff, (rd >> 0) & 0xff);
}
#endif /* UBNT_FLASH_DETECT */


#ifdef ATH_SST_FLASH
void ath_spi_flash_unblock(void)
{
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_WRITE_SR);
	ath_spi_bit_banger(0x0);
	ath_spi_go();
	ath_spi_poll();
}
#endif

#ifdef UBNT_FLASH_DETECT
unsigned long flash_init(void)
{
	u8 idbuf[5];
	u32 jedec;
	spi_flash_t* f;
	flash_info_t* flinfo = &flash_info[0];
	int i;
	u32 sector_size = 0;

#ifndef CONFIG_WASP
#ifdef ATH_SST_FLASH
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x3);
	ath_spi_flash_unblock();
	ath_reg_wr(ATH_SPI_FS, 0);
#else
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x43);
#endif
#endif

	ath_spi_read_id(0x9f, idbuf, 3);
	jedec = (idbuf[0] << 16) | (idbuf[1] << 8) | idbuf[2];

	f = find_flash_data(jedec);
	if (f) {
		flinfo->flash_id = jedec;
		flinfo->size = f->sector_count * f->sector_size;
		flinfo->sector_count = f->sector_count;
	} else {
		/* flash was not detected - take the one, hardcoded
		 * in board info */
        printf("Unrecognized flash 0x%06x\n", jedec);
        flash_get_geom(flinfo);
	}

	sector_size = flinfo->size / flinfo->sector_count;

	/* fill in sector info */
	for (i = 0; i < flinfo->sector_count; ++i) {
		flinfo->start[i] = CFG_FLASH_BASE + (i * sector_size);
		flinfo->protect[i] = 0;
	}

	return flinfo->size;
}

void
ath_spi_flash_print_info(flash_info_t *info)
{
	spi_flash_t* f;
	f = find_flash_data(info->flash_id);
	if (f) {
		printf("%s ", f->name);
	}
	printf("(Id: 0x%06x)\n", info->flash_id);
	printf("\tSize: %ld MB in %d sectors\n",
			info->size >> 20,
			info->sector_count);
}

int
flash_erase(flash_info_t *info, int s_first, int s_last)
{
	int i, sector_size = info->size / info->sector_count;

#ifdef DEBUG
    printf("\nFirst %#x last %#x sector size %#x\n",
           s_first, s_last, sector_size);
#endif

    for (i = s_first; i <= s_last; i++) {
        printf(".");
        ath_spi_sector_erase(i * sector_size);
    }
    ath_spi_done();
    printf(" done\n");

	return 0;
}


#else
unsigned long flash_init(void)
{
#if !(defined(CONFIG_WASP_SUPPORT) || defined(CONFIG_MACH_QCA955x) || defined(CONFIG_MACH_QCA953x) || defined(CONFIG_MACH_QCA956x))
#ifdef ATH_SST_FLASH
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x3);
	ath_spi_flash_unblock();
	ath_reg_wr(ATH_SPI_FS, 0);
#else
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x43);
#endif
#endif
	ath_reg_rmw_set(ATH_SPI_FS, 1);
	ath_spi_read_id();
	ath_reg_rmw_clear(ATH_SPI_FS, 1);

	/*
	 * hook into board specific code to fill flash_info
	 */
	return (flash_get_geom(&flash_info[0]));
}

void
ath_spi_flash_print_info(flash_info_t *info)
{
	printf("The hell do you want flinfo for??\n");
}

int
flash_erase(flash_info_t *info, int s_first, int s_last)
{
	int i, sector_size = info->size / info->sector_count;

	printf("\nFirst %#x last %#x sector size %#x\n",
		s_first, s_last, sector_size);

	for (i = s_first; i <= s_last; i++) {
		printf("\b\b\b\b%4d", i);
		ath_spi_sector_erase(i * sector_size);
	}
	ath_spi_done();
	printf("\n");

	return 0;
}
#endif /* UBNT_FLASH_DETECT */

/*
 * Write a buffer from memory to flash:
 * 0. Assumption: Caller has already erased the appropriate sectors.
 * 1. call page programming for every 256 bytes
 */
#ifdef ATH_SST_FLASH
void
ath_spi_flash_chip_erase(void)
{
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_CHIP_ERASE);
	ath_spi_go();
	ath_spi_poll();
}

int
ath_write_buff(flash_info_t *info, uchar *src, ulong dst, ulong len)
{
	uint32_t val;

	dst = dst - CFG_FLASH_BASE;
	printf("write len: %lu dst: 0x%x src: %p\n", len, dst, src);

	for (; len; len--, dst++, src++) {
		ath_spi_write_enable();	// dont move this above 'for'
		ath_spi_bit_banger(ATH_SPI_CMD_PAGE_PROG);
		ath_spi_send_addr(dst);

		val = *src & 0xff;
		ath_spi_bit_banger(val);

		ath_spi_go();
		ath_spi_poll();
	}
	/*
	 * Disable the Function Select
	 * Without this we can't read from the chip again
	 */
	ath_reg_wr(ATH_SPI_FS, 0);

	if (len) {
		// how to differentiate errors ??
		return ERR_PROG_ERROR;
	} else {
		return ERR_OK;
	}
}
#else
int
ath_write_buff(flash_info_t *info, uchar *source, ulong addr, ulong len)
{
	int total = 0, len_this_lp, bytes_this_page;
	ulong dst;
	uchar *src;

	printf("write addr: %x\n", addr);
	addr = addr - CFG_FLASH_BASE;

	while (total < len) {
		src = source + total;
		dst = addr + total;
		bytes_this_page =
			ATH_SPI_PAGE_SIZE - (addr % ATH_SPI_PAGE_SIZE);
		len_this_lp =
			((len - total) >
			bytes_this_page) ? bytes_this_page : (len - total);
		ath_spi_write_page(dst, src, len_this_lp);
		total += len_this_lp;
	}

	ath_spi_done();

	return 0;
}
#endif

int
write_buff(flash_info_t *info, uchar *source, ulong addr, ulong len)
{
	return(ath_write_buff(info, source, addr - CFG_FLASH_BASE, len));
}

#if ENABLE_EXT_ADDR_SUPPORT
int
read_buff_ext(flash_info_t *info, uchar *buf, ulong offset, ulong len)
{
	ulong    i = 0;
	uint32_t curr_addr = offset;
	uint32_t ori_ear = (uint32_t)ath_spi_rdear();
	uint32_t new_ear;

	while (i < len) {
		new_ear = curr_addr >> 24;
		ath_spi_wrear(new_ear);
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_READ);
		ath_spi_send_addr(curr_addr);
	do{
		ath_spi_delay_8();
		*(buf + i++) = (uchar)(ath_reg_rd(ATH_SPI_RD_STATUS));
			/* update the extended adress update if it's a multiple of 16M */
			if(!((++ curr_addr) & (LEGACY_SPI_ADDR_BOUNDARY - 1))) {
				break;
			}
	}while(i < len);
	ath_spi_go();
	}
	if (new_ear != ori_ear) {
		ath_spi_wrear(ori_ear);
	}
	ath_spi_done();
	return 0;
}

int
write_buff_ext(flash_info_t *info, uchar *source, ulong offset, ulong len)
{
	int      status;
	uint32_t ori_ear = (uint32_t)ath_spi_rdear();
	uint32_t new_ear = 0;
	uint32_t curr_addr = offset;
	uint32_t bytes_this_16M, total = 0;

	while (len) {
		new_ear = curr_addr >> 24;
		ath_spi_wrear(new_ear);
		bytes_this_16M = LEGACY_SPI_ADDR_BOUNDARY - curr_addr % LEGACY_SPI_ADDR_BOUNDARY;
 		bytes_this_16M = (bytes_this_16M < len) ? bytes_this_16M : len;
 		if((status = ath_write_buff(info, source + total, curr_addr, bytes_this_16M)) != ERR_OK) {
			printf("failed to write 0x%x bytes to 0x%x\n", bytes_this_16M, curr_addr);
			break;
		}
 		curr_addr += bytes_this_16M;
		total += bytes_this_16M;
 		len -= bytes_this_16M;
	}
	if (new_ear != ori_ear) {
		ath_spi_wrear(ori_ear);
	}
	ath_spi_done();

	return(status);
}
#endif /* #if ENABLE_EXT_ADDR_SUPPORT */

static void
ath_spi_write_enable()
{
	ath_reg_wr_nf(ATH_SPI_FS, 1);
	ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
	ath_spi_bit_banger(ATH_SPI_CMD_WREN);
	ath_spi_go();
}

static void
ath_spi_poll()
{
	int rd;

	do {
		ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
		ath_spi_bit_banger(ATH_SPI_CMD_RD_STATUS);
		ath_spi_delay_8();
		ath_spi_go();
		rd = (ath_reg_rd(ATH_SPI_RD_STATUS) & 1);
	} while (rd);
}

#if ENABLE_EXT_ADDR_SUPPORT
static void
ath_spi_wrear(uint32_t data)
{
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_WREAR);
	ath_spi_bit_banger((uchar)data);
	ath_spi_go();

	ath_spi_poll();
}

static uchar
ath_spi_rdear(void)
{
	uchar data;

	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_RDEAR);
	ath_spi_delay_8();
	ath_spi_go();
	data = (uchar)(ath_reg_rd(ATH_SPI_RD_STATUS));
	ath_spi_poll();
	return(data);
}

#endif /* #if ENABLE_EXT_ADDR_SUPPORT */

#if !defined(ATH_SST_FLASH)
static void
ath_spi_write_page(uint32_t addr, uint8_t *data, int len)
{
	int i;
	uint8_t ch;

	display(0x77);
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_PAGE_PROG);
	ath_spi_send_addr(addr);

	for (i = 0; i < len; i++) {
		ch = *(data + i);
		ath_spi_bit_banger(ch);
	}

	ath_spi_go();
	display(0x66);
	ath_spi_poll();
	display(0x6d);
}
#endif

static void
ath_spi_sector_erase(uint32_t addr)
{
#if ENABLE_EXT_ADDR_SUPPORT
	uint32_t ori_ear = (uint32_t)ath_spi_rdear();
	uint32_t new_ear = addr >> 24;

	if(new_ear != ori_ear)
		ath_spi_wrear(new_ear);
#endif
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_SECTOR_ERASE);
	ath_spi_send_addr(addr);
	ath_spi_go();
	display(0x7d);
	ath_spi_poll();
#if ENABLE_EXT_ADDR_SUPPORT
	/* recover extended address register */
	if(new_ear != ori_ear)
		ath_spi_wrear(ori_ear);
#endif
}

#ifdef ATH_DUAL_FLASH
void flash_print_info(flash_info_t *info)
{
	ath_spi_flash_print_info(NULL);
	ath_nand_flash_print_info(NULL);
}
#endif
