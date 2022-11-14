#include <common.h>
#include <config.h>
#include <asm/types.h>
#include <flash.h>
#include "flash_oem.h"

void
music_flash_write_oem(board_info_t board_info_new)
{
	u8	oem_buffer[CFG_OEM_LEN] = {0};
	s32 rc = 1;
	s32 rcode = 0;
    board_info_t *board_infop = (board_info_t *)oem_buffer;

#if 0
	board_info_t board_info_new = {
        BOARD_OEM_MAGIC,
        "QCA",
        "UW8513A",
        "v2",
        "SN-00000007",
        {0},
    };
#endif
    puts ("music flash updating new oem info:\n");

    memset(oem_buffer, 0, CFG_OEM_LEN);
	board_info_new.magic = BOARD_OEM_MAGIC;
    memcpy(oem_buffer, &board_info_new, sizeof(board_info_t));
    if(board_infop->magic == BOARD_OEM_MAGIC) {
        debug("oem_id:%s product_id:%s board_version:%s serial_num:%s\n",
                    board_infop->oem_id,
                    board_infop->product_id,
                    board_infop->board_version,
                    board_infop->serial_num);

		if(board_infop->capwap_len) {
	        debug("capwap_base:0x%08x capwap_len:0x%08x\n",
	                    board_infop->capwap_base,
	                    board_infop->capwap_len);
		}
    }

#ifdef CONFIG_ATH_NAND_FL
	if (flash_erase(NULL, CFG_OEM_ADDR, (CFG_OEM_ADDR + CFG_OEM_LEN-1)))
		return;
#else
	if (flash_sect_erase (CFG_OEM_ADDR, (CFG_OEM_ADDR + CFG_OEM_LEN-1)))
		return;
#endif

	puts ("Writing OEM info to Flash... ");

	rc = flash_write((char *)oem_buffer, CFG_OEM_ADDR, CFG_OEM_LEN);
	if (rc != 0) {
		flash_perror (rc);
		rcode = 1;
	} else {
		puts ("done\n");
	}
}
