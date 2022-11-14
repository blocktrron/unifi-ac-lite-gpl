#ifndef _FLASH_H
#define _FLASH_H

#include "music_soc.h"


#ifdef ASDK_SPI_BOOTROM_MODE
#define MUSIC_SPI_FS           0x19000000
#define MUSIC_SPI_CLOCK        0x19000004
#define MUSIC_SPI_WRITE        0x19000008
#define MUSIC_SPI_READ         0x19000000
#define MUSIC_SPI_RD_STATUS    0x1900000c
#else
#define MUSIC_SPI_FS           0x1f000000
#define MUSIC_SPI_CLOCK        0x1f000004
#define MUSIC_SPI_WRITE        0x1f000008
#define MUSIC_SPI_READ         0x1f000000
#define MUSIC_SPI_RD_STATUS    0x1f00000c
#endif


#define MUSIC_SPI_CS_DIS       0x70000
#define MUSIC_SPI_CE_LOW       0x60000
#define MUSIC_SPI_CE_HIGH      0x60100

#define MUSIC_SPI_CMD_WREN         0x06
#define MUSIC_SPI_CMD_RD_STATUS    0x05
#define MUSIC_SPI_CMD_FAST_READ    0x0b
#define MUSIC_SPI_CMD_PAGE_PROG    0x02
#define MUSIC_SPI_CMD_SECTOR_ERASE 0xd8

#define MUSIC_SPI_SECTOR_SIZE      (1024*64)
#define MUSIC_SPI_PAGE_SIZE        256


#define display(_x)     music_reg_wr_nf(0x18040008, (_x))

/*
 * primitives
 */

#define music_be_msb(_val, _i) (((_val) & (1 << (7 - _i))) >> (7 - _i))

#define music_spi_bit_banger(_byte)  do {        \
    int i;                                      \
    for(i = 0; i < 8; i++) {                    \
        music_reg_wr_nf(MUSIC_SPI_WRITE,      \
                        MUSIC_SPI_CE_LOW | music_be_msb(_byte, i));  \
        music_reg_wr_nf(MUSIC_SPI_WRITE,      \
                        MUSIC_SPI_CE_HIGH | music_be_msb(_byte, i)); \
    }       \
}while(0);

#define music_spi_go() do {        \
    music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CE_LOW); \
    music_reg_wr_nf(MUSIC_SPI_WRITE, MUSIC_SPI_CS_DIS); \
}while(0);


#define music_spi_send_addr(addr) do {                    \
    music_spi_bit_banger(((addr & 0xff0000) >> 16));                 \
    music_spi_bit_banger(((addr & 0x00ff00) >> 8));                 \
    music_spi_bit_banger(addr & 0x0000ff);                 \
}while(0);

#define music_spi_delay_8()    music_spi_bit_banger(0)
#define music_spi_done()       music_reg_wr_nf(MUSIC_SPI_FS, 0)

extern unsigned long flash_get_geom (flash_info_t *flash_info);

#endif /*_FLASH_H*/
