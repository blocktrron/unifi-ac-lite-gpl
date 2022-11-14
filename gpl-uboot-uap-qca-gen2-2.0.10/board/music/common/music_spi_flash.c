#include <common.h>
#include <jffs2/jffs2.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include "music_soc.h"
#include "music_spi_flash.h"

#define	EIO		 5	/* I/O error */


/*
 * statics
 */
static void music_spi_write_enable(uint32_t spi_id);
static void music_spi_poll(uint32_t spi_id);
static void music_spi_write_page(uint32_t spi_id, uint32_t addr, uint8_t *data, int len);


struct board_spi_flash_info spi_info;

#define music_spi_send_addr  spi_info.spi_addr_send
#define MUSIC_SPI1_SEL                  1
#define MUSIC_UART1_SEL                 0
#define REG_GLOBAL_CTRL_1               0x18800008
#define GLOBAL_CTRL_1_SPI1_EN_MASK      (1 << 23)

void
music_spi1_uart1_select(uint32_t dev_id)
{
    uint32_t reg_val = 0;

    reg_val = music_reg_rd(REG_GLOBAL_CTRL_1);
    if(MUSIC_SPI1_SEL == dev_id) {
        reg_val |= GLOBAL_CTRL_1_SPI1_EN_MASK;
    } else if (MUSIC_UART1_SEL == dev_id) {
        reg_val &= ~GLOBAL_CTRL_1_SPI1_EN_MASK;
    }
    music_reg_wr(REG_GLOBAL_CTRL_1, reg_val);
    return;
}

void
music_spi_flash_unblock(uint32_t spi_id)
{
    music_spi_write_enable(spi_id);
    music_spi_bit_banger(spi_id,MUSIC_SPI_CMD_WRITE_SR);
    music_spi_bit_banger(spi_id,0x0);
    music_spi_go(spi_id);
    music_spi_poll(spi_id);
    return;
}

static void
music_spi_poll(uint32_t spi_id)
{
    uint32_t rd = 0;

    do {
        music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS);
        music_spi_bit_banger(spi_id,MUSIC_SPI_CMD_RD_STATUS);
        music_spi_delay_8(spi_id);
        rd = (music_reg_rd(MUSIC_SPI_RD_STATUS) & 1);
    }while(rd);

    return;
}

void
music_enter_4bmode(uint32_t spi_id, uint32_t addr)
{
    music_reg_wr_nf(MUSIC_SPI_FS, 1);
    music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS);
    udelay(1);
    music_spi_bit_banger(spi_id,MUSIC_SPI_CMD_EN4B);
    music_spi_go(spi_id);
    music_spi_done();
    return;
}


void
music_exit_4bmode(uint32_t spi_id, uint32_t addr)
{
    music_reg_wr_nf(MUSIC_SPI_FS, 1);
    music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS);
    udelay(1);
    music_spi_bit_banger(spi_id,MUSIC_SPI_CMD_EX4B);
    music_spi_go(spi_id);
    music_spi_done();
    return;
}

void music_spi_three_byte_addr_send(uint32_t spi_id, uint32_t addr)
{
    music_spi_bit_banger(spi_id,((addr & 0xff0000) >> 16));
    music_spi_bit_banger(spi_id,((addr & 0x00ff00) >> 8));
    music_spi_bit_banger(spi_id,addr & 0x0000ff);

    return;
}

void music_spi_four_byte_addr_send(uint32_t spi_id, uint32_t addr)
{
    music_spi_bit_banger(spi_id,((addr & 0xff000000) >> 24));
    music_spi_bit_banger(spi_id,((addr & 0xff0000) >> 16));
    music_spi_bit_banger(spi_id,((addr & 0x00ff00) >> 8));
    music_spi_bit_banger(spi_id,addr & 0x0000ff);

    return;
}


void
read_id(uint32_t spi_id, uint32_t * flash_id)
{
    music_reg_wr(MUSIC_SPI_FS, 1);

    music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS);
    music_spi_bit_banger(spi_id,MUSIC_SPI_CMD_RDID);
    music_spi_delay_8(spi_id);
    music_spi_delay_8(spi_id);
    music_spi_delay_8(spi_id);
    * flash_id = music_reg_rd(MUSIC_SPI_RD_STATUS) & 0xffffff;
    printf("SPI Flash ID: %#x\n", * flash_id);
    music_spi_done();
    return;
}

#define MUSIC_SPI_DEFAULT_READ_FREQUENT     50000000

int32_t
music_spi_base_hw_init(void)
{
    uint32_t spi_clock_div = 0;
    uint32_t reg_val = 0;
    uint32_t bus_freq = music_bus_freq_get();

    reg_val = music_reg_rd(MUSIC_GLB_POS_LOAD_REG);
    if(!(reg_val & GLB_POS_LOAD_SPI_EN_MASK)) {
       printf("Board doesn't enable SPI flash\n");
       return -1;
    }

    spi_clock_div = bus_freq / MUSIC_SPI_DEFAULT_READ_FREQUENT;
    spi_clock_div -= 1;
    spi_clock_div /= 2;

    music_reg_wr(MUSIC_SPI_FS, 1);
    reg_val = spi_clock_div | MUSIC_SPI_CTRL_REMAP_DISABLE_MASK;

    music_reg_wr(MUSIC_SPI_CLOCK, reg_val);
    music_reg_wr(MUSIC_SPI_FS, 0);

    music_spi1_uart1_select(MUSIC_SPI1_SEL);

    return 0;
}
void
music_spi_sw_init(struct board_spi_flash_info * spi_sw_info)
{
    uint32_t reg_val = 0;

    reg_val = music_reg_rd(MUSIC_GLB_POS_LOAD_REG);
    if(reg_val & GLB_POS_LOAD_BOOT_SEL_MASK) {
        spi_info.boot_sel = 1;
        spi_info.base_addr = 0x1f000000;
    } else {
        spi_info.boot_sel = 0;
        spi_info.base_addr = 0x19000000;
    }
    spi_info.valid = 1;

    spi_info.spi_addr_send = spi_sw_info->spi_addr_send;
    spi_info.spi_addr_select = spi_sw_info->spi_addr_select;
    spi_info.spi_addr_unselect = spi_sw_info->spi_addr_unselect;
    spi_info.addr_mode = spi_sw_info->addr_mode;
    spi_info.sector_support = spi_sw_info->sector_support;
    spi_info.page_size = spi_sw_info->page_size;
    spi_info.sector_size = spi_sw_info->sector_size;
    spi_info.block_size = spi_sw_info->block_size;
    spi_info.total_size = spi_sw_info->total_size;

    return;
}

void flash_print_info (flash_info_t *info)
{
    printf("flash_print_info??\n");
}

static void music_spi_sector_erase(uint32_t spi_id, uint32_t addr)
{
    if(spi_info.spi_addr_select) {
        spi_info.spi_addr_select(spi_id, addr);
    }
    music_spi_write_enable(spi_id);
    music_spi_bit_banger(spi_id,MUSIC_SPI_CMD_SECTOR_ERASE);
    music_spi_send_addr(spi_id, addr);
    music_spi_go(spi_id);
    music_spi_poll(spi_id);
    if(spi_info.spi_addr_unselect) {
        spi_info.spi_addr_unselect(spi_id, addr);
    }

    return;
}

static void music_spi_block_erase(uint32_t spi_id, uint32_t addr)
{
    if(spi_info.spi_addr_select) {
        spi_info.spi_addr_select(spi_id, addr);
    }
    music_spi_write_enable(spi_id);
    music_spi_bit_banger(spi_id,MUSIC_SPI_CMD_BLOCK_ERASE);
    music_spi_send_addr(spi_id, addr);
    music_spi_go(spi_id);
    music_spi_poll(spi_id);
    if(spi_info.spi_addr_unselect) {
        spi_info.spi_addr_unselect(spi_id, addr);
    }

    return;
}

int
flash_erase(flash_info_t *info, int s_first, int s_last)
{
    int i = 0, erase_size = 0;
    uint32_t spi_id = SPI_BOOTLOADER_FLASH;

    if(!spi_info.valid) {
        printf("write addr error: return %d", EIO);
        return -EIO;
    }

    if(spi_info.sector_support) {
        erase_size = spi_info.sector_size;
    } else {
        erase_size = spi_info.block_size;
    }

    printf("\nFirst %#x last %#x sector size %#x\n",
           s_first, s_last, erase_size);

    for (i = s_first; i <= s_last; i++) {
        printf("\b\b\b\b%4d", i);
        if(spi_info.sector_support) {
            music_spi_sector_erase(spi_id, i * erase_size);
        } else {
            music_spi_block_erase (spi_id, i * erase_size);
        }
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
    ulong dst = addr;
    uchar *src = NULL;
    uint32_t spi_id = SPI_BOOTLOADER_FLASH;

    if(!spi_info.valid) {
        printf("write addr error: return %d", EIO);
        return -EIO;
    }

    addr = (addr & (~KSEG1)) - ((spi_info.base_addr) & (~KSEG1));

    while(total < len) {
        src              = source + total;
        dst              = addr   + total;
        bytes_this_page  = spi_info.page_size - (addr % spi_info.page_size);
        len_this_lp      = ((len - total) > bytes_this_page) ? bytes_this_page
                                                             : (len - total);
        music_spi_write_page(spi_id, dst, src, len_this_lp);
        total += len_this_lp;
    }

    music_spi_done();

    return 0;
}

void
music_spi_read(uint32_t spi_id, uint32_t addr, uint8_t *data, int len)
{
    uint32_t i = 0;
    int32_t len_temp = (len / 4) * 4;
    int32_t remain = (len % 4);
    uint32_t reg_val = 0;

    if(((addr > 0xb9000000 && (addr + len) < 0xba000000)
        || (addr > 0x99000000 && (addr + len) < 0x9a000000))&&(spi_id == 0)){
        /*enable reads to the SPI space*/
        music_reg_wr(MUSIC_SPI_FS, 0);
        memcpy(data, (uint8_t *)(addr), len);
    } else {
        addr &= (~KSEG1);
        addr -= (spi_info.base_addr & (~KSEG1));

        music_reg_wr(MUSIC_SPI_FS, 1);
        music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS);

        if(spi_info.spi_addr_select) {
            spi_info.spi_addr_select(spi_id, addr);
        }

        music_reg_wr(MUSIC_SPI_FS, 1);

        music_spi_bit_banger(spi_id,MUSIC_SPI_CMD_FASTREAD);
        music_spi_send_addr(spi_id, addr);
        music_spi_delay_8(spi_id);
        for(i = 0;  len_temp > 0;  len_temp -= 4, i +=4) {
            music_spi_delay_8(spi_id);
            music_spi_delay_8(spi_id);
            music_spi_delay_8(spi_id);
            music_spi_delay_8(spi_id);
            reg_val = music_reg_rd(MUSIC_SPI_RD_STATUS);
            *(data + i) = (reg_val) & 0xff;
            *(data + i + 1) = (reg_val >> 8) & 0xff;
            *(data + i + 2) = (reg_val >> 16) & 0xff;
            *(data + i + 3) = (reg_val >> 24) & 0xff;
        }

        for(i = 0; i < 4; i ++) {
            music_spi_delay_8(spi_id);
        }

        reg_val = music_reg_rd(MUSIC_SPI_RD_STATUS);
        i = 0;
        len_temp = (len / 4) * 4;
        while(i < remain) {
            *(data + len_temp + i) = (reg_val >> (i * 8)) & 0xff;
            i ++;
        }

        if(spi_info.spi_addr_unselect) {
            spi_info.spi_addr_unselect(spi_id, addr);
        }
        music_spi_done();
    }

    return;
}


static void
music_spi_write_enable(uint32_t spi_id)
{
    music_reg_wr_nf(MUSIC_SPI_FS, 1);
    music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS);
    music_spi_bit_banger(spi_id, MUSIC_SPI_CMD_WREN);
    music_spi_go(spi_id);
}

static void
music_spi_write_page(uint32_t spi_id, uint32_t addr, uint8_t *data, int len)
{
    int i = 0;
    uint8_t ch = 0;
    int len_temp = (len/4)*4;
    int remain = len%4;

    if(spi_info.spi_addr_select) {
        spi_info.spi_addr_select(spi_id, addr);
    }

    music_spi_write_enable(spi_id);
    music_spi_bit_banger(spi_id,MUSIC_SPI_CMD_PAGE_PROG);
    music_spi_send_addr(spi_id, addr);

    for(i = 0; i < len_temp; i++) {
        ch = *(data + (i / 4) *4 + 3 - (i % 4));
        music_spi_bit_banger(spi_id,ch);
    }

    if(remain) {
        switch(remain) {
        case 1:
            music_spi_bit_banger(spi_id,0xff);
            music_spi_bit_banger(spi_id,0xff);
            music_spi_bit_banger(spi_id,0xff);
            ch = *(data + len_temp+0);
            music_spi_bit_banger(spi_id,ch);
            break;
        case 2:
            music_spi_bit_banger(spi_id,0xff);
            music_spi_bit_banger(spi_id,0xff);
            ch = *(data + len_temp+1);
            music_spi_bit_banger(spi_id,ch);
            ch = *(data + len_temp+0);
            music_spi_bit_banger(spi_id,ch);
            break;
        case 3:
            music_spi_bit_banger(spi_id,0xff);
            ch = *(data + len_temp+2);
            music_spi_bit_banger(spi_id,ch);
            ch = *(data + len_temp+1);
            music_spi_bit_banger(spi_id,ch);
            ch = *(data + len_temp+0);
            music_spi_bit_banger(spi_id,ch);
            break;
	    default:
		    break;
        }
    }

    music_spi_go(spi_id);
    music_spi_poll(spi_id);

    if(spi_info.spi_addr_unselect) {
        spi_info.spi_addr_unselect(spi_id, addr);
    }

    return;
}

int32_t music_spi_base_addr_get(void)
{
    return spi_info.base_addr;
}

void music_spi_reset(uint32_t spi_id)
{
    uint32_t rd = 0;

    music_reg_wr_nf(MUSIC_SPI_FS, 1);
    music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS);

    music_spi_bit_banger(spi_id, MUSIC_SPI_CMD_RST_EN);
    music_spi_bit_banger(spi_id, MUSIC_SPI_CMD_RST);

    music_spi_go(spi_id);
    music_spi_done();
}
