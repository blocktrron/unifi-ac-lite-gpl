#ifndef _MUSIC_SPI_FLASH_H
#define _MUSIC_SPI_FLASH_H

#include "music_soc.h"

#define MUSIC_GLB_POS_LOAD_REG          0x18000008
#define GLB_POS_LOAD_BOOT_SEL_MASK      0x1
#define GLB_POS_LOAD_SPI_EN_MASK        0x10

#ifndef ASDK_SPI_BOOTROM_MODE
#define MUSIC_SPI_BASE                  (0xbf000000)
#define MUSIC_SPI_FS                    MUSIC_SPI_BASE
#define MUSIC_SPI_CLOCK                 (MUSIC_SPI_BASE + 0x4)
#define MUSIC_SPI_WRITE                 (MUSIC_SPI_BASE + 0x8)
#define MUSIC_SPI_READ                  MUSIC_SPI_BASE
#define MUSIC_SPI_RD_STATUS             (MUSIC_SPI_BASE + 0xc)
#else

#define MUSIC_SPI_BASE                  0xb9000000
#define MUSIC_SPI_FS                    MUSIC_SPI_BASE
#define MUSIC_SPI_CLOCK                 (MUSIC_SPI_BASE + 0x4)
#define MUSIC_SPI_WRITE                 (MUSIC_SPI_BASE + 0x8)
#define MUSIC_SPI_READ                  MUSIC_SPI_BASE
#define MUSIC_SPI_RD_STATUS             (MUSIC_SPI_BASE + 0xc)
#endif

#define MUSIC_SPI_CS_DIS                0x70000
#define MUSIC_SPI_CE_LOW                0x60000
#define MUSIC_SPI_CE_HIGH               0x60100

#define MUSIC_SPI_CE1_LOW                0x50000
#define MUSIC_SPI_CE1_HIGH               0x50100
#define MUSIC_SPI_CE2_LOW                0x30000
#define MUSIC_SPI_CE2_HIGH               0x30100

#define MUSIC_SPI_CMD_WREN              0x06
#define MUSIC_SPI_CMD_RD_STATUS         0x05
#define MUSIC_SPI_CMD_FAST_READ         0x0b
#define MUSIC_SPI_CMD_PAGE_PROG         0x02
#define MUSIC_SPI_CMD_SECTOR_ERASE      0x20
#define MUSIC_SPI_CMD_BLOCK_ERASE       0xd8
#define MUSIC_SPI_CMD_FASTREAD          0x0b
#define MUSIC_SPI_CMD_EN4B              0xb7
#define MUSIC_SPI_CMD_EX4B              0xe9
#define MUSIC_SPI_CMD_RDID              0x9f
#define MUSIC_SPI_CMD_RST_EN            0x66
#define MUSIC_SPI_CMD_RST              0x99

#define MUSIC_SPI_CMD_WRITE_SR          0x01
#define MUSIC_SPI_READDATA_CLOCK        40000000

#define MUSIC_SPI_CTRL_REMAP_DISABLE_MASK   0x40


#define SPI_BOOTLOADER_FLASH    0


/*
 * primitives
 */

#define music_be_msb(_val, _i) (((_val) & (1 << (7 - _i))) >> (7 - _i))


#define music_spi_bit_banger(spi_number, _byte) do {        \
    uint32_t _i;                                \
    uint32_t _spi_ce_low = 0;                   \
    uint32_t _spi_ce_high = 0;                  \
    if(spi_number == 0){                        \
        _spi_ce_low = MUSIC_SPI_CE_LOW;         \
        _spi_ce_high = MUSIC_SPI_CE_HIGH;       \
    } else if (spi_number == 1) {               \
        _spi_ce_low = MUSIC_SPI_CE1_LOW;        \
        _spi_ce_high = MUSIC_SPI_CE1_HIGH;      \
    } else if (spi_number == 2) {               \
        _spi_ce_low = MUSIC_SPI_CE2_LOW;        \
        _spi_ce_high = MUSIC_SPI_CE2_HIGH;      \
    }                                           \
    for(_i = 0; _i < 8; _i++) {                 \
        music_reg_wr_nf(MUSIC_SPI_WRITE,    \
                        _spi_ce_low | music_be_msb(_byte, _i)); \
        music_reg_wr_nf(MUSIC_SPI_WRITE,    \
                        _spi_ce_high | music_be_msb(_byte, _i)); \
    }                                           \
}while(0)


#define music_spi_go(spi_number) do {           \
    uint32_t _spi_ce_low = 0;                   \
    if(spi_number == 0){                        \
        _spi_ce_low = MUSIC_SPI_CE_LOW;         \
    } else if (spi_number == 1) {               \
        _spi_ce_low = MUSIC_SPI_CE1_LOW;        \
    } else if (spi_number == 2) {               \
        _spi_ce_low = MUSIC_SPI_CE2_LOW;        \
    }                                           \
    music_reg_wr_nf(MUSIC_SPI_WRITE, _spi_ce_low); \
     udelay(1);   \
    music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS); \
     udelay(1);   \
}while(0)

struct board_spi_flash_info {
    uint32_t valid;
    uint32_t boot_sel;
    uint32_t base_addr;
    uint32_t flash_id;
    uint32_t sector_support;
    uint32_t total_size;
    uint32_t page_size;
    uint32_t sector_size;
    uint32_t block_size;
    uint32_t addr_mode;
    uint32_t remap;
    void (*spi_addr_send)(uint32_t spi_id, uint32_t addr) ;
    void (*spi_addr_select)(uint32_t spi_id, uint32_t addr) ;
    void (*spi_addr_unselect)(uint32_t spi_id, uint32_t addr);
};


struct board_spi_flash_info spi_info;

#define music_spi_delay_8(spi_number)    music_spi_bit_banger(spi_number, 0)
#define music_spi_done()       music_reg_wr_nf(MUSIC_SPI_FS, 0)

extern unsigned long flash_get_geom (flash_info_t *flash_info);


extern void music_spi_three_byte_addr_send(uint32_t spi_id, uint32_t addr);
extern void music_spi_four_byte_addr_send(uint32_t spi_id,uint32_t addr);
extern void music_enter_4bmode(uint32_t spi_id, uint32_t addr);
extern void music_exit_4bmode(uint32_t spi_id,uint32_t addr);
extern void read_id(uint32_t spi_id, uint32_t * flash_id);

#endif /*_FLASH_H*/
