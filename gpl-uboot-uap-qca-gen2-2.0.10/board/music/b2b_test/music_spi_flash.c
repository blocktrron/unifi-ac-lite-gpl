#include <common.h>
#include <jffs2/jffs2.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include "music_soc.h"
#include "music_spi_flash.h"

/*
 * globals
 */
flash_info_t flash_info[CFG_MAX_FLASH_BANKS];


static void
music_spi_flash_unblock()
{
    music_spi_write_enable();
    music_spi_bit_banger(MUSIC_SPI_CMD_WRITE_SR);
    music_spi_bit_banger(0x0);
    music_spi_go();
    music_spi_poll();
}

static void
read_id(void)
{
    u32 rd = 0x777777;

    music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS);
    music_spi_bit_banger(0x9f);
    music_spi_delay_8();
    music_spi_delay_8();
    music_spi_delay_8();
    music_spi_done();
    /* rd = music_reg_rd(MUSIC_SPI_RD_STATUS); */
    rd = music_reg_rd(MUSIC_SPI_READ);
    printf("id read %#x\n", rd);
}

unsigned long
flash_init (void)
{
    int data = 0;
    int spi_clock_div = (music_bus_freq_get() /
						 MUSIC_SPI_READDATA_CLOCK * 2);

	/*start register op*/
    music_reg_wr(MUSIC_SPI_FS, 1);

    /*clear remap*/
    data |= 0x40;

    /*set div*/
    data |= spi_clock_div;

    music_reg_wr(MUSIC_SPI_CLOCK, data);

    music_reg_wr(MUSIC_SPI_FS, 0);

    read_id();

    printf("SPI Flash init done\n");

    /*hook into board specific code to fill flash_info*/

    return (flash_get_geom(&flash_info[0]));
}


void flash_print_info (flash_info_t *info)
{
    printf("flash_print_info??\n");
}

int
flash_erase(flash_info_t *info, int s_first, int s_last)
{
    int i, sector_size = info->size/info->sector_count;

    printf("\nFirst %#x last %#x sector size %#x\n",
           s_first, s_last, sector_size);

    for (i = s_first; i <= s_last; i++) {
        printf("\b\b\b\b%4d", i);
        music_spi_sector_erase(i * sector_size);
    }
    music_spi_done();
    printf("\n");

    return 0;
}

/*
 * Write a buffer from memory to flash:
 * 0. Assumption: Caller has already erased the appropriate sectors.
 * 1. call page programming for every 256 bytes
 */
int
write_buff(flash_info_t *info, uchar *source, ulong addr, ulong len)
{
    int total = 0, len_this_lp, bytes_this_page;
    ulong dst;
    uchar *src;

    printf ("write addr: %x\n", addr);
    addr = addr - CFG_FLASH_BASE;

    while(total < len) {
        src              = source + total;
        dst              = addr   + total;
        bytes_this_page  = MUSIC_SPI_PAGE_SIZE - (addr % MUSIC_SPI_PAGE_SIZE);
        len_this_lp      = ((len - total) > bytes_this_page) ? bytes_this_page
                                                             : (len - total);
        music_spi_write_page(dst, src, len_this_lp);
        total += len_this_lp;
    }

    music_spi_done();

    return 0;
}

static void
music_spi_write_enable(void)
{
    music_reg_wr_nf(MUSIC_SPI_FS, 1);
    music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS);
    music_spi_bit_banger(MUSIC_SPI_CMD_WREN);
    music_spi_go();
}

static void
music_spi_poll(void)
{
    int rd;

    do {
        music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS);
        music_spi_bit_banger(MUSIC_SPI_CMD_RD_STATUS);
        music_spi_delay_8();
        rd = (music_reg_rd(MUSIC_SPI_RD_STATUS) & 1);
    }while(rd);
}

static void
music_spi_write_page(uint32_t addr, uint8_t *data, int len)
{
    int i;
    uint8_t ch;
    int len_temp = (len/4)*4;
    int remain = len%4;

    music_spi_write_enable();
    music_spi_bit_banger(MUSIC_SPI_CMD_PAGE_PROG);
    music_spi_send_addr(addr);

    for(i = 0; i < len_temp; i++) {
        ch = *(data + (i / 4) *4 + 3 - (i % 4));
        music_spi_bit_banger(ch);
    }

    if(remain) {
        switch(remain) {
        case 1:
            music_spi_bit_banger(0xff);
            music_spi_bit_banger(0xff);
            music_spi_bit_banger(0xff);
            ch = *(data + len_temp+0);
            music_spi_bit_banger(ch);
            break;
        case 2:
            music_spi_bit_banger(0xff);
            music_spi_bit_banger(0xff);
            ch = *(data + len_temp+1);
            music_spi_bit_banger(ch);
            ch = *(data + len_temp+0);
            music_spi_bit_banger(ch);
            break;
        case 3:
            music_spi_bit_banger(0xff);
            ch = *(data + len_temp+2);
            music_spi_bit_banger(ch);
            ch = *(data + len_temp+1);
            music_spi_bit_banger(ch);
            ch = *(data + len_temp+0);
            music_spi_bit_banger(ch);
            break;
	    default:
		    break;
        }
    }

    music_spi_go();
    music_spi_poll();
}

static void
music_spi_sector_erase(uint32_t addr)
{
    music_spi_write_enable();
    music_spi_bit_banger(MUSIC_SPI_CMD_SECTOR_ERASE);
    music_spi_send_addr(addr);
    music_spi_go();
    music_spi_poll();
}
