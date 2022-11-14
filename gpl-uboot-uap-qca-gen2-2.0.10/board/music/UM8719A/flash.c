#include <common.h>
#include <config.h>
#include <asm/types.h>
#include <flash.h>
#include "../common/music_spi_flash.h"
/*
 * sets up flash_info and returns size of FLASH (bytes)
 */
#define SPI_MX25L25635E_ID      0xc22019
#define SPI_M25Q128_ID          0x12018
#define SPI_MX25L6406E_ID       0xc22017


flash_info_t flash_info[CFG_MAX_FLASH_BANKS];
unsigned long
flash_get_geom (flash_info_t *flash_info)
{
    int i;

    /* XXX this is hardcoded until we figure out how to read flash id */

    flash_info->flash_id  = FLASH_M25P64;
    flash_info->size = CFG_FLASH_SIZE; /* bytes */
    flash_info->sector_count = flash_info->size/CFG_FLASH_SECTOR_SIZE;

    for (i = 0; i < flash_info->sector_count; i++) {
        flash_info->start[i] = CFG_FLASH_BASE + (i * CFG_FLASH_SECTOR_SIZE);
        flash_info->protect[i] = 0;
    }

    printf ("flash size %d, sector count = %d\n", flash_info->size, flash_info->sector_count);
    return (flash_info->size);

}

#ifndef CONFIG_ATH_NAND_FL
unsigned long
flash_init (void)
{
    struct board_spi_flash_info spi_init_info;
    uint32_t flash_id = 0;
    uint32_t spi_id = 0;
    uint32_t i = 0;
    uint32_t soft_sector_size = 0;
    memset(& spi_init_info, 0, sizeof(spi_init_info));

    music_spi_base_hw_init();
    read_id(spi_id, & flash_id);
    if(flash_id == SPI_MX25L25635E_ID) {
        spi_init_info.flash_id  = flash_id;
        spi_init_info.page_size = 0x100;
        spi_init_info.sector_support = 1;
        spi_init_info.sector_size = 0x1000;
        spi_init_info.addr_mode = 4;
        spi_init_info.spi_addr_send = music_spi_four_byte_addr_send;
        spi_init_info.block_size = 0x10000;
        spi_init_info.spi_addr_select = music_enter_4bmode;
        spi_init_info.spi_addr_unselect = music_exit_4bmode;
        spi_init_info.total_size = 0x2000000;
    } else if (flash_id == SPI_MX25L6406E_ID) {
        spi_init_info.sector_support = 1;
        spi_init_info.flash_id  = flash_id;
        spi_init_info.page_size = 0x100;
        spi_init_info.sector_size = 0x1000;
        spi_init_info.block_size = 0x40000;
        spi_init_info.addr_mode = 3;
        spi_init_info.spi_addr_send = music_spi_three_byte_addr_send;
        spi_init_info.total_size = 0x800000;
        spi_init_info.spi_addr_select = NULL;
        spi_init_info.spi_addr_unselect = NULL;
    } else if (flash_id == SPI_M25Q128_ID) {
        spi_init_info.sector_support = 0;
        spi_init_info.flash_id  = flash_id;
        spi_init_info.page_size = 0x100;
        spi_init_info.sector_size = 0x1000;
        spi_init_info.block_size = 0x40000;
        spi_init_info.addr_mode = 3;
        spi_init_info.spi_addr_send = music_spi_three_byte_addr_send;
        spi_init_info.total_size = 0x1000000;
        spi_init_info.spi_addr_select = NULL;
        spi_init_info.spi_addr_unselect = NULL;
    }

    music_spi_sw_init(&spi_init_info);
    printf("SPI Flash init done\n");

    flash_info->flash_id  = flash_id;
    flash_info->size = spi_init_info.total_size; /* bytes */
    if(spi_init_info.sector_support) {
        soft_sector_size = spi_init_info.sector_size;
        flash_info->sector_count = flash_info->size/spi_init_info.sector_size;
    } else {
        soft_sector_size = spi_init_info.block_size;
        flash_info->sector_count = flash_info->size/spi_init_info.block_size;
    }

    if(flash_info->sector_count > CFG_MAX_FLASH_SECT) {
        printf("flash sector count too much \n");
    }

    for (i = 0; i < flash_info->sector_count; i++) {
        flash_info->start[i] = CFG_FLASH_BASE + (i * soft_sector_size);
        flash_info->protect[i] = 0;
    }

    printf ("flash size 0x%x, sector count = %d\n", flash_info->size, flash_info->sector_count);
    return (flash_info->size);
}
#endif
