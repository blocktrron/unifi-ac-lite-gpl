#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <config.h>
#include <version.h>
#include "music_soc.h"

#define MUSIC_DDR_SIZE 0x10000000

void irq_ctrl_setup(void)
{
	music_reg_wr(0xb8050004, 0);
    music_reg_wr(0xb8050008, 0);
    music_reg_wr(0xb8050010, 0);
    music_reg_wr(0xb8050014, 0);
    music_reg_wr(0xb8050018, 0);
    music_reg_wr(0xb805001c, 0);
    music_reg_wr(0xb8050020, 0);
    music_reg_wr(0xb8050024, 0);
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
    printf("UM8719B U-boot\n");
    printf("CPU Freq:%dMHz\n", (music_bus_freq_get()*2)/1000000);
#ifdef CONFIG_BUILD_VERSION
	printf("ASDK v%s\n", CONFIG_BUILD_VERSION);
#endif
    music_switch_main();
    return 0;
}

void pci_init_board(void)
{

}
