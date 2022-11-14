#include <common.h>
#include <command.h>
#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/types.h>
#include "music_nand_flash.h"
#include "music_soc.h"

flash_info_t		flash_info[CFG_MAX_FLASH_BANKS];
ath_nand_flinfo_t	nf_info;
int			ath_nand_init_done = 0;

unsigned get_nand_status(void)
{
	unsigned	rddata;

	rddata = music_reg_rd(MUSIC_NF_RST_REG);
	while (rddata != 1) {
		rddata = music_reg_rd(MUSIC_NF_RST_REG);
	}

	music_reg_wr(MUSIC_NF_RST, 0x07024);  // READ STATUS
	rddata = music_reg_rd(MUSIC_NF_RD_STATUS);

	return rddata;
}

/*
 * globals
 */
void read_id(ath_nand_id_t *id)
{
	flush_cache(id, 8);
	music_reg_wr(MUSIC_NF_DMA_ADDR, (unsigned)virt_to_phys(id));
	music_reg_wr(MUSIC_NF_ADDR0_0, 0x0);
	music_reg_wr(MUSIC_NF_ADDR0_1, 0x0);
	music_reg_wr(MUSIC_NF_DMA_COUNT, 0x8);
	music_reg_wr(MUSIC_NF_PG_SIZE, 0x8);
	music_reg_wr(MUSIC_NF_DMA_CTRL, 0xcc);
	music_reg_wr(MUSIC_NF_RST, 0x9061); 	// READ ID

	get_nand_status();	// Don't check for status

	flush_cache(id, 8);

	/*printf(	"____ %s _____\n"
		"  vid did wc  ilp nsp ct  dp  sa1 org bs  sa0 ss  ps  res1 pls pn  res2\n"
		"0x%3x %3x %3x %3x %3x %3x %3x %3x %3x %3x %3x %3x %3x %3x  %3x %3x %3x\n"
		"-------------\n", __func__,
			id->vid, id->did, id->wc, id->ilp,
			id->nsp, id->ct, id->dp, id->sa1,
			id->org, id->bs, id->sa0, id->ss,
			id->ps, id->res1, id->pls, id->pn,
			id->res2);*/
}

void flash_print_info (flash_info_t *info)
{
	ath_nand_id_t	id;
	read_id(&id);
}


void nand_block_erase(unsigned addr0, unsigned addr1)
{
	unsigned	rddata;

    music_reg_wr(MUSIC_NF_ADDR0_0, addr0);
	music_reg_wr(MUSIC_NF_ADDR0_1, addr1);
	music_reg_wr(MUSIC_NF_RST, 0xd0600e);	// BLOCK ERASE

    rddata = 0;
	do {
		rddata = music_reg_rd(MUSIC_NF_RST_REG);
	}while (rddata != 1);


	rddata = (get_nand_status() & MUSIC_NF_READ_STATUS_MASK);
	if (rddata != MUSIC_NF_STATUS_OK) {
		printf("Erase Failed. status = 0x%x", rddata);
	}
}

int
flash_erase(flash_info_t *info, int s_first, int s_last)
{
	int i, j;

	printf("\nFirst %#x last %#x sector size %#x\n",
		s_first, s_last, MUSIC_NF_BLK_SIZE);

	for (j = 0, i = s_first; i < s_last; j++, i += MUSIC_NF_BLK_SIZE) {
		ulong ba0, ba1;

        int page_offset = 16;
        int page_size = 11;
        ba0 = (i >> page_size)  << page_offset;
        ba1 = (i >> page_size) >> (32 - page_offset);

#if 1
		printf("\b\b\b\b%4d", j);
#else
		printf("flash_erase: 0x%x 0x%x%08x\n", i, ba1, ba0);
#endif
		nand_block_erase(ba0, ba1);

	}

	printf("\n");

	return 0;
}

unsigned
rw_page(int rd, unsigned addr0, unsigned addr1, unsigned count, unsigned *buf)
{
	unsigned	rddata, tries = 3;

    while(tries--) {
	music_reg_wr(MUSIC_NF_ADDR0_0, addr0);
	music_reg_wr(MUSIC_NF_ADDR0_1, addr1);
	music_reg_wr(MUSIC_NF_DMA_ADDR, (unsigned)buf);
	music_reg_wr(MUSIC_NF_DMA_COUNT, count);
	music_reg_wr(MUSIC_NF_PG_SIZE, count);

	if (rd) {	// Read Page
		music_reg_wr(MUSIC_NF_DMA_CTRL, 0xcc);
		music_reg_wr(MUSIC_NF_RST, 0x30006a);
	} else {	// Write Page
		music_reg_wr(MUSIC_NF_DMA_CTRL, 0x8c);
		music_reg_wr(MUSIC_NF_RST, 0x10804c);
	}

	rddata = (get_nand_status() & MUSIC_NF_READ_STATUS_MASK);
	if (rddata != MUSIC_NF_STATUS_OK) {
		/*if(rd == 1)  {
                printf("Read");
            } else {
                printf("Write");
             }
		printf(" Failed. status = 0x%x\n", rddata);*/
	} else {
            return rddata;
        }
    }
}

int
flash_init_in_env(void)
{
	music_reg_wr(MUSIC_NF_CTRL,	(5) |
					(1 << 6) |
					(3 << 8) |
					(1 << 11));

	// TIMINGS_ASYN Reg Settings
	music_reg_wr(MUSIC_NF_TIMINGS_ASYN, MUSIC_NF_TIMING_ASYN);

	// NAND Mem Control Reg
	music_reg_wr(MUSIC_NF_MEM_CTRL, 0xff00);

	// Reset Command
	music_reg_wr(MUSIC_NF_RST, 0xff00);

	nf_info.erasesize_shift = MUSIC_NF_BLK_SIZE_S;
	nf_info.erasesize_mask = MUSIC_NF_BLK_SIZE_M;
	nf_info.writesize = 2048;
	nf_info.erasesize	= (1 << nf_info.erasesize_shift);
	nf_info.oobsize		=  64;//(nf_info.writesize / 512) * (8 << id->ss);

}

/* max page and spare size */
uchar partial_read_buffer[4096 + 256];

int
rw_buff(int rd, uchar *buf, ulong addr, ulong len, unsigned pgsz)
{
	const unsigned	nbs = 65536;
	ulong		total = 0;
	unsigned	ret = MUSIC_NF_STATUS_OK,
			dbs = nf_info.writesize;
	uchar		*p;

	dbs = pgsz;

	if (ath_nand_init_done!=1) {
		/*
		 * env_init() tries to read the flash. However, it is
		 * called before flash_init. Hence read with defaults
		 */
	    flash_init_in_env();
		dbs = pgsz = nf_info.writesize;
	}

	while (total < len && ret == MUSIC_NF_STATUS_OK) {
		ulong ba0, ba1;

		p = buf;

        int page_offset = 16;
        int page_size = 11;
        ba0 = (addr >> page_size)  << page_offset;
        ba1 = (addr >> page_size) >> (32 - page_offset);

		if ((len - total) < pgsz) {
			p = partial_read_buffer;
			if (!rd) {
				memcpy(p, buf, len - total);
				printf("memcpy(%p, %p, %d)\n", p, buf, len-total);
			}
		}

		if (!rd) {
			//printf("rw_buff(%d): 0x%x 0x%x%08x buf = %p pgsz %u\n",
			//		rd, addr, ba1, ba0, p, pgsz);
		}

		flush_cache(p, pgsz);	// for writes...
		ret = rw_page(rd, ba0, ba1, pgsz, virt_to_phys(p));
		flush_cache(p, pgsz);	// for read

		total += pgsz;
		buf += pgsz;
		addr += dbs;
	}

	if (rd && p == partial_read_buffer) {
		buf -= dbs;
		total -= dbs;
		memcpy(buf, p, len - total);
	}

	return (ret != MUSIC_NF_STATUS_OK);
}

int
write_buff(flash_info_t *info, uchar *buf, ulong addr, ulong len)
{
	if (info) {
		return rw_buff(0 /* write */, buf, addr, len, nf_info.writesize + nf_info.oobsize);
	} else {
		return rw_buff(0 /* write */, buf, addr, len, nf_info.writesize);
	}
}

int
read_buff(flash_info_t *info, uchar *buf, ulong addr, ulong len)
{
	if (info) {
		return rw_buff(1 /* read */, buf, addr, len, nf_info.writesize + nf_info.oobsize);
	} else {
		return rw_buff(1 /* read */, buf, addr, len, nf_info.writesize);
	}
}


unsigned long
flash_init(void)
{
    if(ath_nand_init_done) {
        return;
    }

	uint64_t ath_plane_size[] =
		{ 64 << 20, 1 << 30, 2 << 30, 4 << 30, 8 << 30 };
	ath_nand_id_t		*id;

    // Control Reg Setting
	music_reg_wr(MUSIC_NF_CTRL,
	                (MUSIC_NF_ADDR_CYCLES_NUM) |
					(1 << 6) |  //blk size 64
					(3 << 8) |  //page 2048
					(MUSIC_NF_CUSTOM_SIZE_EN << 11));

	// TIMINGS_ASYN Reg Settings
	music_reg_wr(MUSIC_NF_TIMINGS_ASYN, MUSIC_NF_TIMING_ASYN);

	// NAND Mem Control Reg
	music_reg_wr(MUSIC_NF_MEM_CTRL, 0xff00);

	// Reset Command
	music_reg_wr(MUSIC_NF_RST, 0xff00);

	id = &nf_info.id;
	read_id(id);

	nf_info.size		= ath_plane_size[id->pls] << id->pn;
    printf("nf_info.size:%x\n", nf_info.size);

	nf_info.erasesize_shift = MUSIC_NF_BLK_SIZE_S;
	nf_info.erasesize_mask = MUSIC_NF_BLK_SIZE_M;
	nf_info.writesize = MUSIC_NF_PAGE_SIZE;
	nf_info.erasesize	= (1 << nf_info.erasesize_shift);
	nf_info.oobsize		=  64;//(nf_info.writesize / 512) * (8 << id->ss);
    ath_nand_init_done = 1;
	/*
	 * hook into board specific code to fill flash_info
	 */
	printf("NAND Flash init done\n");

	return (flash_get_geom(&flash_info[0]));
}
