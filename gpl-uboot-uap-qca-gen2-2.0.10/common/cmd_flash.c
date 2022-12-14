/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

/*
 * FLASH support
 */
#include <common.h>
#include <command.h>

#ifdef CONFIG_HAS_DATAFLASH
#include <dataflash.h>
#endif

#if (CONFIG_COMMANDS & CFG_CMD_FLASH)

#if (CONFIG_COMMANDS & CFG_CMD_JFFS2) && defined(CONFIG_JFFS2_CMDLINE)
#include <jffs2/jffs2.h>
#include <ath_flash.h>
#include "ssd1317fb-splash.h"

/* parition handling routines */
int mtdparts_init(void);
int id_parse(const char *id, const char **ret_id, u8 *dev_type, u8 *dev_num);
int find_dev_and_part(const char *id, struct mtd_device **dev,
		u8 *part_num, struct part_info **part);
#endif

extern flash_info_t flash_info[];	/* info for FLASH chips */

/*
 * The user interface starts numbering for Flash banks with 1
 * for historical reasons.
 */

/*
 * this routine looks for an abbreviated flash range specification.
 * the syntax is B:SF[-SL], where B is the bank number, SF is the first
 * sector to erase, and SL is the last sector to erase (defaults to SF).
 * bank numbers start at 1 to be consistent with other specs, sector numbers
 * start at zero.
 *
 * returns:	1	- correct spec; *pinfo, *psf and *psl are
 *			  set appropriately
 *		0	- doesn't look like an abbreviated spec
 *		-1	- looks like an abbreviated spec, but got
 *			  a parsing error, a number out of range,
 *			  or an invalid flash bank.
 */
static int
abbrev_spec (char *str, flash_info_t ** pinfo, int *psf, int *psl)
{
	flash_info_t *fp;
	int bank, first, last;
	char *p, *ep;

	if ((p = strchr (str, ':')) == NULL)
		return 0;
	*p++ = '\0';

	bank = simple_strtoul (str, &ep, 10);
	if (ep == str || *ep != '\0' ||
		bank < 1 || bank > CFG_MAX_FLASH_BANKS ||
		(fp = &flash_info[bank - 1])->flash_id == FLASH_UNKNOWN)
		return -1;

	str = p;
	if ((p = strchr (str, '-')) != NULL)
		*p++ = '\0';

	first = simple_strtoul (str, &ep, 10);
	if (ep == str || *ep != '\0' || first >= fp->sector_count)
		return -1;

	if (p != NULL) {
		last = simple_strtoul (p, &ep, 10);
		if (ep == p || *ep != '\0' ||
			last < first || last >= fp->sector_count)
			return -1;
	} else {
		last = first;
	}

	*pinfo = fp;
	*psf = first;
	*psl = last;

	return 1;
}

/*
 * This function computes the start and end addresses for both
 * erase and protect commands. The range of the addresses on which
 * either of the commands is to operate can be given in two forms:
 * 1. <cmd> start end - operate on <'start',  'end')
 * 2. <cmd> start +length - operate on <'start', start + length)
 * If the second form is used and the end address doesn't fall on the
 * sector boundary, than it will be adjusted to the next sector boundary.
 * If it isn't in the flash, the function will fail (return -1).
 * Input:
 *    arg1, arg2: address specification (i.e. both command arguments)
 * Output:
 *    addr_first, addr_last: computed address range
 * Return:
 *    1: success
 *   -1: failure (bad format, bad address).
*/
static int
addr_spec(char *arg1, char *arg2, ulong *addr_first, ulong *addr_last)
{
	char len_used = 0; /* indicates if the "start +length" form used */
	char *ep;

	*addr_first = simple_strtoul(arg1, &ep, 16);
	if (ep == arg1 || *ep != '\0')
		return -1;

	if (arg2 && *arg2 == '+'){
		len_used = 1;
		++arg2;
	}

	*addr_last = simple_strtoul(arg2, &ep, 16);
	if (ep == arg2 || *ep != '\0')
		return -1;

	if (len_used){
		char found = 0;
		ulong bank;

		/*
		 * *addr_last has the length, compute correct *addr_last
		 * XXX watch out for the integer overflow! Right now it is
		 * checked for in both the callers.
		 */
		*addr_last = *addr_first + *addr_last - 1;

		/*
		 * It may happen that *addr_last doesn't fall on the sector
		 * boundary. We want to round such an address to the next
		 * sector boundary, so that the commands don't fail later on.
		 */
		/* find the end addr of the sector where the *addr_last is */
		for (bank = 0; bank < CFG_MAX_FLASH_BANKS && !found; ++bank){
			int i;
			flash_info_t *info = &flash_info[bank];
			for (i = 0; i < info->sector_count && !found; ++i){
				/* get the end address of the sector */
				ulong sector_end_addr;
				if (i == info->sector_count - 1){
					sector_end_addr =
						info->start[0] + info->size - 1;
				} else {
					sector_end_addr =
						info->start[i+1] - 1;
				}
				if (*addr_last <= sector_end_addr &&
						*addr_last >= info->start[i]){
					/* sector found */
					found = 1;
					/* adjust *addr_last if necessary */
					if (*addr_last < sector_end_addr){
						*addr_last = sector_end_addr;
					}
				}
			} /* sector */
		} /* bank */
		if (!found){
			/* error, addres not in flash */
			printf("Error: end address (0x%08lx) not in flash!\n",
								*addr_last);
			return -1;
		}
	} /* "start +length" from used */

	return 1;
}

static int
flash_fill_sect_ranges (ulong addr_first, ulong addr_last,
			int *s_first, int *s_last,
			int *s_count )
{
	flash_info_t *info;
	ulong bank;
	int rcode = 0;

	*s_count = 0;

	for (bank=0; bank < CFG_MAX_FLASH_BANKS; ++bank) {
		s_first[bank] = -1;	/* first sector to erase	*/
		s_last [bank] = -1;	/* last  sector to erase	*/
	}

	for (bank=0,info=&flash_info[0];
	     (bank < CFG_MAX_FLASH_BANKS) && (addr_first <= addr_last);
	     ++bank, ++info) {
		ulong b_end;
		int sect;
		short s_end;

		if (info->flash_id == FLASH_UNKNOWN) {
			continue;
		}

		b_end = info->start[0] + info->size - 1;	/* bank end addr */
		s_end = info->sector_count - 1;			/* last sector   */


		for (sect=0; sect < info->sector_count; ++sect) {
			ulong end;	/* last address in current sect	*/

			end = (sect == s_end) ? b_end : info->start[sect + 1] - 1;

			if (addr_first > end)
				continue;
			if (addr_last < info->start[sect])
				continue;

			if (addr_first == info->start[sect]) {
				s_first[bank] = sect;
			}
			if (addr_last  == end) {
				s_last[bank]  = sect;
			}
		}
		if (s_first[bank] >= 0) {
			if (s_last[bank] < 0) {
				if (addr_last > b_end) {
					s_last[bank] = s_end;
				} else {
					puts ("Error: end address"
						" not on sector boundary\n");
					rcode = 1;
					break;
				}
			}
			if (s_last[bank] < s_first[bank]) {
				puts ("Error: end sector"
					" precedes start sector\n");
				rcode = 1;
				break;
			}
			sect = s_last[bank];
			addr_first = (sect == s_end) ? b_end + 1: info->start[sect + 1];
			(*s_count) += s_last[bank] - s_first[bank] + 1;
		} else if (addr_first >= info->start[0] && addr_first < b_end) {
			puts ("Error: start address not on sector boundary\n");
			rcode = 1;
			break;
		} else if (s_last[bank] >= 0) {
			puts ("Error: cannot span across banks when they are"
			       " mapped in reverse order\n");
			rcode = 1;
			break;
		}
	}

	return rcode;
}



#if defined(ENABLE_LCM)

#define SSD1309_CMD    0x0
#define SSD1309_DAT    0x1

#define lcm_spi_bit_banger(_byte) do {        \
  int i;                \
  for(i = 0; i < 8; i++) {          \
    ath_reg_wr_nf(ATH_SPI_WRITE,        \
      0x70000 | ath_be_msb(_byte, i));   \
    ath_reg_wr_nf(ATH_SPI_WRITE,        \
      0x70100 | ath_be_msb(_byte, i));  \
  }               \
} while (0)

void
lcm_send_byte(uint8_t data, uint8_t chCmd)
{
  int i;
  if(chCmd == SSD1309_CMD )
    ath_reg_rmw_clear(ATH_GPIO_OUT, (1 << 20));
  else
    ath_reg_rmw_set(ATH_GPIO_OUT, (1 << 20));

  ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
  ath_reg_rmw_clear(ATH_GPIO_OUT, (1 << 11));

    for(i = 0; i < 8; i++) {
      ath_reg_wr_nf(ATH_SPI_WRITE,(0x70000 | ath_be_msb(data, i)));
      ath_reg_wr_nf(ATH_SPI_WRITE,(0x70100 | ath_be_msb(data, i)));
    }

  ath_reg_rmw_set(ATH_GPIO_OUT, (1 << 11));
}

void
lcm_send_byte_array(uint8_t *data, int len, uint8_t chCmd)
{
  int i;
  uchar ch;

  for(i = 0; i < len; i++) {
    ch=*(data+i);
    lcm_send_byte(ch, chCmd);
  }

  ath_reg_rmw_set(ATH_GPIO_OUT, (1 << 11));
}

void draw_logo()
{
    int invert = 0;
    int width = 128;
    int height = 64;
    int x, y, i, im, j, jm, pel;
    unsigned int len = width * height / 8;
    u8 *array = (u8 *) malloc(len);

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            // index and bitmask for XBM format in buf
            i = (y * 16) + (x / 8);
            im = 1 << (x % 8);

            // index and bitmask for OLED format in array
            j = (127 - x) * 8 + y / 8;
            jm = 1 << (y % 8);

            pel = ((ssd1317z_splash[i] & im)) ? 1 : 0;
            array[j] |= (pel ^ invert) ? jm : 0;
        }
    }

  lcm_send_byte_array(array,128 * 8,SSD1309_DAT);
}

void
lcm_init()
{
	unsigned	clk;
    uchar lcm_init_cmd[]={
        0xFD,0x12,0x81,0x20,0xc8,0xa1,0x3f,0xd3,
        0x50,0xd5,0xa0,0xd9,0x22,0xda,0x12,0xdb,
        0x30,0x8d,0x00,0x20,0x01,0x21,0x0,0x7f,
        0x22,0x0,0x7,0xaf
    };
    // lcm cs => gpio11
    ath_reg_rmw_clear(ATH_GPIO_OE, (1 << 11));
    ath_reg_rmw_set(ATH_GPIO_OUT, (1 << 11));
    // lcm d/c => gpio20 (cmd: 0x0, data: 0x1)
    ath_reg_rmw_clear(ATH_GPIO_OE, (1 << 20));
    ath_reg_rmw_clear(ATH_GPIO_OUT, (1 << 20));
    // lcm reset => gpio21
    ath_reg_rmw_clear(ATH_GPIO_OE, (1 << 21));
    ath_reg_rmw_clear(ATH_GPIO_OUT, (1 << 21));
    udelay(4);
    ath_reg_rmw_set(ATH_GPIO_OUT, (1 << 21));
    udelay(4);
    // lcm display enable => gpio5
    ath_reg_rmw_clear(ATH_GPIO_OE, (1 << 5));
    ath_reg_rmw_set(ATH_GPIO_OUT, (1 << 5));
    ath_reg_wr_nf(ATH_SPI_FS, 1);
    lcm_send_byte_array(lcm_init_cmd,sizeof(lcm_init_cmd)/sizeof(uchar),SSD1309_CMD);
}

void spi_config_revert()
{
  // revert value in ATH_SPI_FS
  ath_reg_wr_nf(ATH_SPI_FS, 0);
}

int do_lcm ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    lcm_init();
    draw_logo();

    // lock LCM command
    lcm_send_byte(0xFD,SSD1309_CMD);
    lcm_send_byte(0x16,SSD1309_CMD);
    spi_config_revert();

	return 0;
}
#endif // #if defined(ENABLE_LCM)


#ifndef COMPRESSED_UBOOT
int do_flinfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong bank;

#ifdef CONFIG_HAS_DATAFLASH
	dataflash_print_info();
#endif

	if (argc == 1) {	/* print info for all FLASH banks */
		for (bank=0; bank <CFG_MAX_FLASH_BANKS; ++bank) {
			printf ("\nBank # %ld: ", bank+1);

			flash_print_info (&flash_info[bank]);
		}
		return 0;
	}

	bank = simple_strtoul(argv[1], NULL, 16);
	if ((bank < 1) || (bank > CFG_MAX_FLASH_BANKS)) {
		printf ("Only FLASH Banks # 1 ... # %d supported\n",
			CFG_MAX_FLASH_BANKS);
		return 1;
	}
	printf ("\nBank # %ld: ", bank);
	flash_print_info (&flash_info[bank-1]);
	return 0;
}
#endif /* #ifndef COMPRESSED_UBOOT */

#if ENABLE_EXT_ADDR_SUPPORT
int do_flread_ext (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	flash_info_t *info;
	ulong buf, offset, len;
	int   bank = 1;

	if (argc < 4) {
		printf ("Usage:\n%s\n%s\n", cmdtp->usage, cmdtp->help);
		return 1;
	}
	buf = simple_strtoul(argv[1], NULL, 16);
	offset = simple_strtoul(argv[2], NULL, 16);
	len = simple_strtoul(argv[3], NULL, 16);
	if (argc > 4) {
		bank = simple_strtoul(argv[4], NULL, 16);
		if ((bank < 1) || (bank > CFG_MAX_FLASH_BANKS)) {
			printf ("Only FLASH Banks # 1 ... # %d supported\n",
			CFG_MAX_FLASH_BANKS);
			return 1;
		}
	}
	info = &flash_info[bank - 1];

	if ((offset + len) > info->size){
		printf ("Bad parameters, 'offset' + 'len' should < 0x%x\n", info->size);
		return 1;
	}
	return(read_buff_ext(info, (uchar *)buf, offset, len));
}

int do_flwrite_ext (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    flash_info_t *info;
    ulong source, offset, len;
	int   bank = 1;

    if (argc < 4) {
        printf ("Usage:\n%s\n%s\n", cmdtp->usage, cmdtp->help);
		return 1;
	}
	source = simple_strtoul(argv[1], NULL, 16);
	offset = simple_strtoul(argv[2], NULL, 16);
	len = simple_strtoul(argv[3], NULL, 16);
	if (argc > 4) {
		bank = simple_strtoul(argv[4], NULL, 16);
		if ((bank < 1) || (bank > CFG_MAX_FLASH_BANKS)) {
			printf ("Only FLASH Banks # 1 ... # %d supported\n",
			CFG_MAX_FLASH_BANKS);
			return 1;
		}
	}

	info = &flash_info[bank - 1];
	if ((offset + len) > info->size){
		printf ("Bad parameters, 'offset' + 'len' should < 0x%x\n", info->size);
	    return 1;
	}

	return(write_buff_ext(info, (uchar *)source, offset, len));
}

int do_flerase_ext (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	flash_info_t *info;
	ulong offset, len;
	int sect_first, sect_last, sect_size;
	int   bank = 1;

    if (argc < 3) {
	     printf ("Usage:\n%s\n%s\n", cmdtp->usage, cmdtp->help);
	     return 1;
	}
	offset = simple_strtoul(argv[1], NULL, 16);
	len = simple_strtoul(argv[2], NULL, 16);

	if (argc > 4) {
		bank = simple_strtoul(argv[4], NULL, 16);
		if ((bank < 1) || (bank > CFG_MAX_FLASH_BANKS)) {
			printf ("Only FLASH Banks # 1 ... # %d supported\n",
			CFG_MAX_FLASH_BANKS);
			return 1;
		}
	}

	info = &flash_info[bank - 1];
	if ((offset + len) > info->size){
		printf ("Bad parameters, 'offset' + 'len' should < 0x%x\n", info->size);
		return 1;
	}
	sect_size = info->size / info->sector_count;
	sect_first = offset / sect_size;
	sect_last = (offset + len - 1) / sect_size;
	printf("flash size:0x%x offset:0x%x len:0x%x sect_size:0x%x first:0x%x last:0x%x\n",
			info->size, offset, len, sect_size, sect_first, sect_last);

	return(flash_erase(info, sect_first, sect_last));
}
#endif /* #if ENABLE_EXT_ADDR_SUPPORT */

int do_flerase (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	flash_info_t *info;
	ulong bank, addr_first, addr_last;
	int n, sect_first, sect_last;
#if (CONFIG_COMMANDS & CFG_CMD_JFFS2) && defined(CONFIG_JFFS2_CMDLINE)
	struct mtd_device *dev;
	struct part_info *part;
	u8 dev_type, dev_num, pnum;
#endif
	int rcode = 0;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (strcmp(argv[1], "all") == 0) {
		for (bank=1; bank<=CFG_MAX_FLASH_BANKS; ++bank) {
			printf ("Erase Flash Bank # %ld ", bank);
			info = &flash_info[bank-1];
			rcode = flash_erase (info, 0, info->sector_count-1);
		}
		return rcode;
	}

	if ((n = abbrev_spec(argv[1], &info, &sect_first, &sect_last)) != 0) {
		if (n < 0) {
			puts ("Bad sector specification\n");
			return 1;
		}
		printf ("Erase Flash Sectors %d-%d in Bank # %d ",
			sect_first, sect_last, (info-flash_info)+1);
		rcode = flash_erase(info, sect_first, sect_last);
		return rcode;
	}

#if (CONFIG_COMMANDS & CFG_CMD_JFFS2) && defined(CONFIG_JFFS2_CMDLINE)
	/* erase <part-id> - erase partition */
	if ((argc == 2) && (id_parse(argv[1], NULL, &dev_type, &dev_num) == 0)) {
		mtdparts_init();
		if (find_dev_and_part(argv[1], &dev, &pnum, &part) == 0) {
			if (dev->id->type == MTD_DEV_TYPE_NOR) {
				bank = dev->id->num;
				info = &flash_info[bank];
				addr_first = part->offset + info->start[0];
				addr_last = addr_first + part->size - 1;

				printf ("Erase Flash Parition %s, "
						"bank %d, 0x%08lx - 0x%08lx ",
						argv[1], bank, addr_first,
						addr_last);

				rcode = flash_sect_erase(addr_first, addr_last);
				return rcode;
			}

			printf("cannot erase, not a NOR device\n");
			return 1;
		}
	}
#endif

	if (argc != 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (strcmp(argv[1], "bank") == 0) {
		bank = simple_strtoul(argv[2], NULL, 16);
		if ((bank < 1) || (bank > CFG_MAX_FLASH_BANKS)) {
			printf ("Only FLASH Banks # 1 ... # %d supported\n",
				CFG_MAX_FLASH_BANKS);
			return 1;
		}
		printf ("Erase Flash Bank # %ld ", bank);
		info = &flash_info[bank-1];
		rcode = flash_erase (info, 0, info->sector_count-1);
		return rcode;
	}
	if (addr_spec(argv[1], argv[2], &addr_first, &addr_last) < 0){
		printf ("Bad address format\n");
		return 1;
	}

	if (addr_first >= addr_last) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	rcode = flash_sect_erase(addr_first, addr_last);
	return rcode;
}

int flash_sect_erase (ulong addr_first, ulong addr_last)
{
	flash_info_t *info;
	ulong bank;
#ifdef CFG_MAX_FLASH_BANKS_DETECT
	int s_first[CFG_MAX_FLASH_BANKS_DETECT], s_last[CFG_MAX_FLASH_BANKS_DETECT];
#else
	int s_first[CFG_MAX_FLASH_BANKS], s_last[CFG_MAX_FLASH_BANKS];
#endif
	int erased = 0;
	int planned;
	int rcode = 0;

	rcode = flash_fill_sect_ranges (addr_first, addr_last,
					s_first, s_last, &planned );

	if (planned && (rcode == 0)) {
		for (bank=0,info=&flash_info[0];
		     (bank < CFG_MAX_FLASH_BANKS) && (rcode == 0);
		     ++bank, ++info) {
			if (s_first[bank]>=0) {
				erased += s_last[bank] - s_first[bank] + 1;
				debug ("Erase Flash from 0x%08lx to 0x%08lx "
					"in Bank # %ld ",
					info->start[s_first[bank]],
					(s_last[bank] == info->sector_count) ?
						info->start[0] + info->size - 1:
						info->start[s_last[bank]+1] - 1,
					bank+1);
				rcode = flash_erase (info, s_first[bank], s_last[bank]);
			}
		}
		printf ("Erased %d sectors\n", erased);
	} else if (rcode == 0) {
		puts ("Error: start and/or end address"
			" not on sector boundary\n");
		rcode = 1;
	}
	return rcode;
}

#ifndef COMPRESSED_UBOOT
int do_protect (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	flash_info_t *info;
	ulong bank, addr_first, addr_last;
	int i, p, n, sect_first, sect_last;
#if (CONFIG_COMMANDS & CFG_CMD_JFFS2) && defined(CONFIG_JFFS2_CMDLINE)
	struct mtd_device *dev;
	struct part_info *part;
	u8 dev_type, dev_num, pnum;
#endif
	int rcode = 0;
#ifdef CONFIG_HAS_DATAFLASH
	int status;
#endif

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (strcmp(argv[1], "off") == 0) {
		p = 0;
	} else if (strcmp(argv[1], "on") == 0) {
		p = 1;
	} else {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

#ifdef CONFIG_HAS_DATAFLASH
	if ((strcmp(argv[2], "all") != 0) && (strcmp(argv[2], "bank") != 0)) {
		addr_first = simple_strtoul(argv[2], NULL, 16);
		addr_last  = simple_strtoul(argv[3], NULL, 16);

		if (addr_dataflash(addr_first) && addr_dataflash(addr_last)) {
			status = dataflash_real_protect(p,addr_first,addr_last);
			if (status < 0){
				puts ("Bad DataFlash sector specification\n");
				return 1;
			}
			printf("%sProtect %d DataFlash Sectors\n",
				p ? "" : "Un-", status);
			return 0;
		}
	}
#endif

	if (strcmp(argv[2], "all") == 0) {
		for (bank=1; bank<=CFG_MAX_FLASH_BANKS; ++bank) {
			info = &flash_info[bank-1];
			if (info->flash_id == FLASH_UNKNOWN) {
				continue;
			}
			printf ("%sProtect Flash Bank # %ld\n",
				p ? "" : "Un-", bank);

			for (i=0; i<info->sector_count; ++i) {
#if defined(CFG_FLASH_PROTECTION)
				if (flash_real_protect(info, i, p))
					rcode = 1;
				putc ('.');
#else
				info->protect[i] = p;
#endif	/* CFG_FLASH_PROTECTION */
			}
		}

#if defined(CFG_FLASH_PROTECTION)
		if (!rcode) puts (" done\n");
#endif	/* CFG_FLASH_PROTECTION */

		return rcode;
	}

	if ((n = abbrev_spec(argv[2], &info, &sect_first, &sect_last)) != 0) {
		if (n < 0) {
			puts ("Bad sector specification\n");
			return 1;
		}
		printf("%sProtect Flash Sectors %d-%d in Bank # %d\n",
			p ? "" : "Un-", sect_first, sect_last,
			(info-flash_info)+1);
		for (i = sect_first; i <= sect_last; i++) {
#if defined(CFG_FLASH_PROTECTION)
			if (flash_real_protect(info, i, p))
				rcode =  1;
			putc ('.');
#else
			info->protect[i] = p;
#endif	/* CFG_FLASH_PROTECTION */
		}

#if defined(CFG_FLASH_PROTECTION)
		if (!rcode) puts (" done\n");
#endif	/* CFG_FLASH_PROTECTION */

		return rcode;
	}

#if (CONFIG_COMMANDS & CFG_CMD_JFFS2) && defined(CONFIG_JFFS2_CMDLINE)
	/* protect on/off <part-id> */
	if ((argc == 3) && (id_parse(argv[2], NULL, &dev_type, &dev_num) == 0)) {
		mtdparts_init();
		if (find_dev_and_part(argv[2], &dev, &pnum, &part) == 0) {
			if (dev->id->type == MTD_DEV_TYPE_NOR) {
				bank = dev->id->num;
				info = &flash_info[bank];
				addr_first = part->offset + info->start[0];
				addr_last = addr_first + part->size - 1;

				printf ("%sProtect Flash Parition %s, "
						"bank %d, 0x%08lx - 0x%08lx\n",
						p ? "" : "Un", argv[1],
						bank, addr_first, addr_last);

				rcode = flash_sect_protect (p, addr_first, addr_last);
				return rcode;
			}

			printf("cannot %sprotect, not a NOR device\n",
					p ? "" : "un");
			return 1;
		}
	}
#endif

	if (argc != 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (strcmp(argv[2], "bank") == 0) {
		bank = simple_strtoul(argv[3], NULL, 16);
		if ((bank < 1) || (bank > CFG_MAX_FLASH_BANKS)) {
			printf ("Only FLASH Banks # 1 ... # %d supported\n",
				CFG_MAX_FLASH_BANKS);
			return 1;
		}
		printf ("%sProtect Flash Bank # %ld\n",
			p ? "" : "Un-", bank);
		info = &flash_info[bank-1];

		if (info->flash_id == FLASH_UNKNOWN) {
			puts ("missing or unknown FLASH type\n");
			return 1;
		}
		for (i=0; i<info->sector_count; ++i) {
#if defined(CFG_FLASH_PROTECTION)
			if (flash_real_protect(info, i, p))
				rcode =  1;
			putc ('.');
#else
			info->protect[i] = p;
#endif	/* CFG_FLASH_PROTECTION */
		}

#if defined(CFG_FLASH_PROTECTION)
		if (!rcode) puts (" done\n");
#endif	/* CFG_FLASH_PROTECTION */

		return rcode;
	}

	if (addr_spec(argv[2], argv[3], &addr_first, &addr_last) < 0){
		printf("Bad address format\n");
		return 1;
	}

	if (addr_first >= addr_last) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	rcode = flash_sect_protect (p, addr_first, addr_last);
	return rcode;
}


int flash_sect_protect (int p, ulong addr_first, ulong addr_last)
{
	flash_info_t *info;
	ulong bank;
#ifdef CFG_MAX_FLASH_BANKS_DETECT
	int s_first[CFG_MAX_FLASH_BANKS_DETECT], s_last[CFG_MAX_FLASH_BANKS_DETECT];
#else
	int s_first[CFG_MAX_FLASH_BANKS], s_last[CFG_MAX_FLASH_BANKS];
#endif
	int protected, i;
	int planned;
	int rcode;

	rcode = flash_fill_sect_ranges( addr_first, addr_last, s_first, s_last, &planned );

	protected = 0;

	if (planned && (rcode == 0)) {
		for (bank=0,info=&flash_info[0]; bank < CFG_MAX_FLASH_BANKS; ++bank, ++info) {
			if (info->flash_id == FLASH_UNKNOWN) {
				continue;
			}

			if (s_first[bank]>=0 && s_first[bank]<=s_last[bank]) {
				debug ("%sProtecting sectors %d..%d in bank %ld\n",
					p ? "" : "Un-",
					s_first[bank], s_last[bank], bank+1);
				protected += s_last[bank] - s_first[bank] + 1;
				for (i=s_first[bank]; i<=s_last[bank]; ++i) {
#if defined(CFG_FLASH_PROTECTION)
					if (flash_real_protect(info, i, p))
						rcode = 1;
					putc ('.');
#else
					info->protect[i] = p;
#endif	/* CFG_FLASH_PROTECTION */
				}
			}
		}
#if defined(CFG_FLASH_PROTECTION)
		puts (" done\n");
#endif	/* CFG_FLASH_PROTECTION */

		printf ("%sProtected %d sectors\n",
			p ? "" : "Un-", protected);
	} else if (rcode == 0) {
		puts ("Error: start and/or end address"
			" not on sector boundary\n");
		rcode = 1;
	}
	return rcode;
}
#endif /* #ifndef COMPRESSED_UBOOT */

/**************************************************/
#if (CONFIG_COMMANDS & CFG_CMD_JFFS2) && defined(CONFIG_JFFS2_CMDLINE)
# define TMP_ERASE	"erase <part-id>\n    - erase partition\n"
# define TMP_PROT_ON	"protect on <part-id>\n    - protect partition\n"
# define TMP_PROT_OFF	"protect off <part-id>\n    - make partition writable\n"
#else
# define TMP_ERASE	/* empty */
# define TMP_PROT_ON	/* empty */
# define TMP_PROT_OFF	/* empty */
#endif

#ifndef COMPRESSED_UBOOT
#if defined(ENABLE_LCM)
U_BOOT_CMD(
	lcm,    2,    1,    do_lcm,
	"flinfo N\n    - draw lcm\n",
);
#endif

U_BOOT_CMD(
	flinfo,    2,    1,    do_flinfo,
	"flinfo  - print FLASH memory information\n",
	"\n    - print information for all FLASH memory banks\n"
	"flinfo N\n    - print information for FLASH memory bank # N\n"
);

U_BOOT_CMD(
	protect,  4,  1,   do_protect,
	"protect - enable or disable FLASH write protection\n",
	"on  start end\n"
	"    - protect FLASH from addr 'start' to addr 'end'\n"
	"protect on start +len\n"
	"    - protect FLASH from addr 'start' to end of sect "
	"w/addr 'start'+'len'-1\n"
	"protect on  N:SF[-SL]\n"
	"    - protect sectors SF-SL in FLASH bank # N\n"
	"protect on  bank N\n    - protect FLASH bank # N\n"
	TMP_PROT_ON
	"protect on  all\n    - protect all FLASH banks\n"
	"protect off start end\n"
	"    - make FLASH from addr 'start' to addr 'end' writable\n"
	"protect off start +len\n"
	"    - make FLASH from addr 'start' to end of sect "
	"w/addr 'start'+'len'-1 wrtable\n"
	"protect off N:SF[-SL]\n"
	"    - make sectors SF-SL writable in FLASH bank # N\n"
	"protect off bank N\n    - make FLASH bank # N writable\n"
	TMP_PROT_OFF
	"protect off all\n    - make all FLASH banks writable\n"
);

#endif /* #ifndef COMPRESSED_UBOOT */

U_BOOT_CMD(
	erase,   3,   1,  do_flerase,
	"erase   - erase FLASH memory\n",
	"start end\n"
	"    - erase FLASH from addr 'start' to addr 'end'\n"
	"erase start +len\n"
	"    - erase FLASH from addr 'start' to the end of sect "
	"w/addr 'start'+'len'-1\n"
	"erase N:SF[-SL]\n    - erase sectors SF-SL in FLASH bank # N\n"
	"erase bank N\n    - erase FLASH bank # N\n"
	TMP_ERASE
	"erase all\n    - erase all FLASH banks\n"
);

#if ENABLE_EXT_ADDR_SUPPORT
U_BOOT_CMD(
	read_ext,   5,   1,  do_flread_ext,
	"read_ext   - read SPI NOR FLASH memory(0-32M space)\n",
	"read_ext dst offset(offset of the flash) len [N]\n"
	"    - read 'len' bytes from FLASH bank #N addr 'start' to memory addr 'dst'\n"
	"    - 'N' is optional, default the first bank\n"
    );

U_BOOT_CMD(
	erase_ext,   4,   1,  do_flerase_ext,
	"erase_ext   - erase SPI NOR FLASH memory(0-32M space)\n",
	"erase_ext start(offset of the flash) len [N]\n"
	"    - erase FLASH bank #N from addr 'start' to the end of sect addr 'start'+'len'-1\n"
	"    - 'N' is optional, default the first bank\n"
	);

U_BOOT_CMD(
	write_ext,   5,   1,  do_flwrite_ext,
	"write_ext   - write SPI NOR FLASH memory(0-32M space)\n",
	"write_ext src dst(offset of the flash) len [N]\n"
	"    - copy 'len' bytes from memory addr 'src' to FLASH bank #N addr 'start'\n"
	"    - 'N' is optional, default the first bank\n"
	);
#endif /* #if ENABLE_EXT_ADDR_SUPPORT */

#undef	TMP_ERASE
#undef	TMP_PROT_ON
#undef	TMP_PROT_OFF

#endif	/* CFG_CMD_FLASH */
