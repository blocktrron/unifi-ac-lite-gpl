#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <config.h>
#include <version.h>
#include "music_soc.h"

#define MUSIC_DDR_SIZE 0x2000000

int soc_bus_freq_get(void)
{
    return 25000000;
}

int irq_ctrl_setup(void)
{
    music_reg_wr(0xb8050004, 0x41060);
    music_reg_wr(0xb8050008, 0x00038);
    music_reg_wr(0xb805001c, 0x01000);
    music_reg_wr(0xb8050020, 0x00060);
    music_reg_wr(0xb8050024, 0x40000);
}

int
music_mem_config(void)
{
#ifndef CONFIG_ATH_NAND_FL

#if (TEXT_BASE == 0xbf000000) ||  (TEXT_BASE == 0xb9000000)
    ddr_init();
#endif

#endif
    irq_ctrl_setup();

    return MUSIC_DDR_SIZE;
}

long int initdram(int board_type)
{
    return (music_mem_config());
}

int checkboard (void)
{
    printf("Music-Emu U-boot\n");
    return 0;
}

void pci_init_board(void)
{

}
