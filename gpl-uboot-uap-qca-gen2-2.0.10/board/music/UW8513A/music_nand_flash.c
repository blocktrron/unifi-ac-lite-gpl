#include <common.h>
#include <command.h>
#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/types.h>
#include "music_nand_flash.h"
#include "music_soc.h"

#include <linux/mtd/mtd.h>

#ifdef MUSIC_KDEBUG
#define MUSIC_KDEBUG printf
#else
#define MUSIC_KDEBUG(...)
#endif
typedef unsigned char sa_u8_t;
typedef int	sa_i32_t;
typedef unsigned int sa_u32_t;
typedef size_t sa_size_t;

struct music_nand_flash_info {
    sa_u32_t total_size;
    sa_u32_t page_size;
    sa_u32_t block_size;
    sa_u32_t addr_cyc;
    sa_u32_t time_asyn;
    sa_u32_t oob_size;
    sa_u32_t ecc_offset;
    sa_u32_t ecc_en;
};

struct music_nand_flash_info *music_nand_flash;

struct music_nand_flash_info music_nand_K9XXG08UXM = {
    .total_size = 2048 * 64 * 2048, //128m
    .page_size = 2048,
    .block_size = 64,
    .oob_size = 64,
    .ecc_en = 0,
    .addr_cyc = 5,
};

struct mtd_info music_nand_info[CFG_MAX_NAND_DEVICE];


#define do_div(n,base) ({ \
	int __res; \
	__res = ((unsigned long) n) % (unsigned) base; \
	n = ((unsigned long) n) / (unsigned) base; \
	__res; \
})

static sa_i32_t music_nand_erase(struct mtd_info *mtd, struct erase_info *instr);
static sa_i32_t music_nand_read(struct mtd_info *mtd, loff_t from, sa_size_t len,
                sa_size_t * retlen, u_char * buf);
static sa_i32_t music_nand_write(struct mtd_info *mtd, loff_t to, sa_size_t len,
                 sa_size_t * retlen, const u_char * buf);
static sa_i32_t music_nand_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops);
static sa_i32_t music_nand_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops);
static sa_i32_t music_nand_block_isbad(struct mtd_info *mtd, loff_t ofs);
static sa_i32_t music_nand_block_markbad(struct mtd_info *mtd, loff_t ofs);

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
    struct mtd_info *mtd = &music_nand_info[0];

	printf("\nFirst %#x last %#x sector size %#x\n",
		s_first, s_last, MUSIC_NF_BLK_SIZE);

	for (j = 0, i = s_first; i < s_last; j++, i += MUSIC_NF_BLK_SIZE) {
		ulong ba0, ba1;

        int page_offset = 16;
        int page_size = 11;
        ba0 = (i >> page_size)  << page_offset;
        ba1 = (i >> page_size) >> (32 - page_offset);
#if 0
        if((i % nf_info.erasesize == 0) && (music_nand_block_isbad(mtd, i) != 0))
        {
            printf("skip erase bad block at address [0x%x]\n", i);
            continue;
        }
#endif
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
    struct mtd_info *mtd = &music_nand_info[0];
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
	music_nand_flash = &music_nand_K9XXG08UXM;

	mtd->name = "music-nand0";

	//mtd->writesize = music_nand_flash->page_size;
    //mtd->writesize_shift = 11;
    //mtd->writesize_mask = (mtd->writesize - 1);
    mtd->oobblock = music_nand_flash->page_size;
    mtd->erasesize = (music_nand_flash->page_size * music_nand_flash->block_size);
    //mtd->erasesize_shift = 17;
    //mtd->erasesize_mask = (mtd->erasesize - 1);
    mtd->size = music_nand_flash->total_size;
    mtd->oobsize = music_nand_flash->oob_size;
    mtd->oobavail = mtd->oobsize;

	mtd->erase = music_nand_erase;
	mtd->read = music_nand_read;
	mtd->write = music_nand_write;
	mtd->read_oob = music_nand_read_oob;
	mtd->write_oob = music_nand_write_oob;
	mtd->block_isbad = music_nand_block_isbad;
    mtd->block_markbad = music_nand_block_markbad;
	printf("flash_init_in_env\n");
}

/* max page and spare size */
uchar partial_read_buffer[4096 + 256];
char    temp_buf[0x1000];

int
rw_buff(int rd, uchar *buf, ulong addr, ulong len, unsigned pgsz)
{
	const unsigned	nbs = 65536;
	ulong		total = 0;
	unsigned	ret = MUSIC_NF_STATUS_OK,
			dbs = nf_info.writesize;
	uchar		*p;
    uchar   mark_value[2];
    char *temp_buf_p = &temp_buf;
    struct mtd_info *mtd = &music_nand_info[0];

	//dbs = pgsz; //bug fix

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
				//printf("memcpy(%p, %p, %d)\n", p, buf, len-total);
			}
		}

		if (!rd) {
			//printf("rw_buff(%d): 0x%x 0x%x%08x buf = %p pgsz %u\n",
			//		rd, addr, ba1, ba0, p, pgsz);
		}

        //jump the first 1MB range, there are bios, oem, enviroment, uboot images.
        if((addr >= 0x100000) && (addr % nf_info.erasesize == 0) && (music_nand_block_isbad(mtd, addr) != 0))
        //if((addr % nf_info.erasesize == 0) && (music_nand_block_isbad(mtd, addr) != 0))
        {
            printf("skip access bad block at address[0x%x]\n", addr);
            addr += nf_info.erasesize;
            continue;
        }
        if((pgsz > nf_info.writesize) && (rd == 0)) //will write oob
        {
#if 0
            //check whether file system image is correct
            if((*(buf+nf_info.writesize)==0xff) && (*(buf+nf_info.writesize+1)==0xff))
                rd = rd;
            else if((*(buf+nf_info.writesize)!=0x0) || (*(buf+nf_info.writesize+1)!=0x0))
                printf("addr:0x%x checked is not correct\n", addr+nf_info.writesize);
#endif
            mark_value[0] = 0xff;
            mark_value[1] = 0xff;
            memcpy(temp_buf_p+2, buf + nf_info.writesize, 62);
            *temp_buf_p = mark_value[0];
            *(temp_buf_p+1) = mark_value[1];
            memcpy(buf+nf_info.writesize, temp_buf_p, 64);
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
    sa_u32_t reg_val = 0;
    if(ath_nand_init_done) {
        return;
    }

	uint64_t ath_plane_size[] =
		{ 64 << 20, 1 << 30, 2 << 30, 4 << 30, 8 << 30 };
	ath_nand_id_t		*id;

    /* Reset Command */
    music_reg_wr(MUSIC_NF_RST, 0xff00);

    /* Clear ECC error flag */
    reg_val = music_reg_rd(MUSIC_NF_ECC_CTRL);
    reg_val &= (~((1<<2) | (1<<1) | (1<<0)));
    music_reg_wr(MUSIC_NF_ECC_CTRL, reg_val);
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
	struct mtd_info *mtd = &music_nand_info[0];
	music_nand_flash = &music_nand_K9XXG08UXM;

	mtd->name = "music-nand0";

	//mtd->writesize = music_nand_flash->page_size;
    //mtd->writesize_shift = 11;
    //mtd->writesize_mask = (mtd->writesize - 1);
    mtd->oobblock = music_nand_flash->page_size;
    mtd->erasesize = (music_nand_flash->page_size * music_nand_flash->block_size);
    //mtd->erasesize_shift = 17;
    //mtd->erasesize_mask = (mtd->erasesize - 1);
    mtd->size = music_nand_flash->total_size;
    mtd->oobsize = music_nand_flash->oob_size;
    mtd->oobavail = mtd->oobsize;

	mtd->erase = music_nand_erase;
	mtd->read = music_nand_read;
	mtd->write = music_nand_write;
	mtd->read_oob = music_nand_read_oob;
	mtd->write_oob = music_nand_write_oob;
	mtd->block_isbad = music_nand_block_isbad;
    mtd->block_markbad = music_nand_block_markbad;
	/*
	 * hook into board specific code to fill flash_info
	 */
	printf("NAND Flash init done\n");

	return (flash_get_geom(&flash_info[0]));
}
int cmd_nand_erase(unsigned int s_first, unsigned int len)
{
	int ret;
	struct mtd_info *mtd = &music_nand_info[0];
	struct erase_info instr;

	instr.addr = s_first;
	instr.len = len;
	instr.callback = NULL;

	printf("start = %x\tlen = %x\n", instr.addr, instr.len);
	ret = music_nand_erase(mtd, &instr);

	if (ret < 0) {
		printf("Erase Error!\n");
	} else {
		printf("done.\n");
	}

	return ret;
}

int cmd_nand_read(uchar *buf, ulong from, ulong len)
{
	int ret;
	sa_size_t retlen;
	struct mtd_info *mtd = &music_nand_info[0];

	printf("buf = %x\tfrom = %x\tlen = %x\n", buf, from, len);
	ret = music_nand_read(mtd, from, len, &retlen, buf);

	if (ret < 0) {
		printf("Read Error!\n");
	} else {
		printf("done.\nretlen = %x\n", retlen);
	}

	return ret;
}

int cmd_nand_write(uchar *buf, ulong to, ulong len)
{
	int ret;
	sa_size_t retlen;
	struct mtd_info *mtd = &music_nand_info[0];

	printf("buf = %x\tfrom = %x\tlen = %x\n", buf, to, len);
	ret = music_nand_write(mtd, to, len, &retlen, buf);

	if (ret < 0) {
		printf("Write Error!\n");
	} else {
		printf("done.\nretlen = %x\n", retlen);
	}

	return ret;
}

int cmd_nand_dump(ulong off, int oob_only)
{
	int i, ret;
	uchar *buf, *p;
	sa_u32_t c;
	struct mtd_info *mtd = &music_nand_info[0];

	buf = malloc(mtd->oobblock + mtd->oobsize);

	ret = rw_buff(1 /* read */, buf, off, mtd->oobblock + mtd->oobsize, mtd->oobblock + mtd->oobsize);

	if (ret < 0) {
		printf("Error (%d) reading page %08x\n", ret, off);
		free(buf);
		return 1;
	}

	c = off & (mtd->oobblock - 1);
	printf("Page %08x dump:\n", off-c);

	i = mtd->oobblock >> 4; p = buf;
	while (i--) {
		if (!oob_only) {
			printf( "\t%02x %02x %02x %02x %02x %02x %02x %02x"
				"  %02x %02x %02x %02x %02x %02x %02x %02x\n",
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
				p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
		}
		p += 16;
	}

	puts("OOB:\n");
	i = mtd->oobsize >> 3;
	while (i--) {
		printf( "\t%02x %02x %02x %02x %02x %02x %02x %02x\n",
			p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		p += 8;
	}
	free(buf);

	return ret;
}

sa_u32_t music_nand_status(void)
{
    sa_u32_t rddata = 0;
    sa_u32_t busydata = 0;

    do {
        busydata = music_reg_rd(MUSIC_NF_RST_REG);
    } while (busydata != 0x1);

    music_reg_wr(MUSIC_NF_RST, 0x07024);    /* READ STATUS */

    busydata = 0;
    do {
        busydata = music_reg_rd(MUSIC_NF_RST_REG);
    } while (busydata != 0x1);

    rddata = music_reg_rd(MUSIC_NF_RD_STATUS);

    return rddata;
}


static sa_i32_t music_nand_get_reg_addr(loff_t addr, sa_u32_t * addr0, sa_u32_t * addr1)
{
    sa_u8_t addr_cyc[5] = { 0 };
    loff_t temp = 0;

    memset(addr_cyc, 0, sizeof (addr_cyc));
    temp = addr;
    do_div(temp, music_nand_flash->page_size);

    addr_cyc[2] = temp & 0xff;
    addr_cyc[3] = (temp >> 8) & 0xff;
    addr_cyc[4] = (temp >> 16) & 0xff;

    *addr0 =
        addr_cyc[0] | (addr_cyc[1] << 8) | (addr_cyc[2] << 16) | (addr_cyc[3] <<
                                                                  24);
    *addr1 = addr_cyc[4];

    return 0;
}


void music_nand_block_erase(sa_u32_t addr0, sa_u32_t addr1)
{
    sa_u32_t rddata = 0;

    music_reg_wr(MUSIC_NF_ADDR0_0, addr0);
    music_reg_wr(MUSIC_NF_ADDR0_1, addr1);
    music_reg_wr(MUSIC_NF_RST, 0xd0600e);   /* BLOCK ERASE */

    rddata = music_nand_status() & MUSIC_NF_READ_STATUS_MASK;

    if (rddata != MUSIC_NF_STATUS_OK) {
        printf("MUSIC NAND block erase Failed: status = 0x%x", rddata);
    }

    return;
}


sa_u32_t music_nand_rw_page(sa_i32_t rd, sa_u32_t addr0,
                   sa_u32_t addr1, sa_u32_t count, sa_u8_t * buf)
{
    sa_u32_t rddata = 0;
    sa_u8_t *err[] = { "Write", "Read" };

    music_reg_wr(MUSIC_NF_ADDR0_0, addr0);
    music_reg_wr(MUSIC_NF_ADDR0_1, addr1);
    music_reg_wr(MUSIC_NF_DMA_ADDR, (sa_u32_t) buf);
    music_reg_wr(MUSIC_NF_DMA_COUNT, count);
    music_reg_wr(MUSIC_NF_PG_SIZE, count);

    if (rd) {
        /* Read Page */
        music_reg_wr(MUSIC_NF_DMA_CTRL, 0xcc);
        music_reg_wr(MUSIC_NF_RST, 0x30006a);
    } else {
        /* Write Page */
        music_reg_wr(MUSIC_NF_DMA_CTRL, 0x8c);
        music_reg_wr(MUSIC_NF_RST, 0x10804c);
    }

    rddata = music_nand_status() & MUSIC_NF_READ_STATUS_MASK;
    if (rddata != MUSIC_NF_STATUS_OK) {
        printf("MUSIC nand %s page failed: status = 0x%x\n",
                   err[rd], rddata);
        return -5;
    }

    return 0;
}


static sa_i32_t music_nand_rw_buff(struct mtd_info *mtd, sa_i32_t rd, sa_u8_t * buf,
                   loff_t addr, sa_size_t len, sa_size_t * iodone)
{
    sa_u32_t iolen = 0, ret = 0;
    sa_u8_t *pa = NULL;
    sa_u8_t *err[] = { "write", "read" };

    pa = malloc(sizeof (sa_u8_t) * (mtd->oobblock + mtd->oobsize));

    if (NULL == pa) {
        printf("MUSIC NAND %s buffer failed: memory exhausted \n", err[rd]);
        free(pa);
        return -12;
    }

    *iodone = 0;

    while (len) {
        sa_u32_t c = 0, addr0 = 0, addr1 = 0;

        music_nand_get_reg_addr(addr, &addr0, &addr1);

        c = addr & (mtd->oobblock - 1);

        if (c) {
            iolen = mtd->oobblock - c;
        } else {
            iolen = mtd->oobblock;
        }

        if (len < iolen) {
            iolen = len;
        }

        if (!rd) {

			flush_cache(pa, mtd->oobblock);	// for writes...
            ret = music_nand_rw_page(1, addr0, addr1, mtd->oobblock, virt_to_phys(pa));
			flush_cache(pa, mtd->oobblock);	// for writes...

            memcpy((pa + c), buf, iolen);
        }

		flush_cache(pa, mtd->oobblock);	// for writes...
        ret = music_nand_rw_page(rd, addr0, addr1, mtd->oobblock, virt_to_phys(pa));
		flush_cache(pa, mtd->oobblock);	// for read

        if (rd) {
            memcpy(buf, (pa + c), iolen);
        }

        if (ret != 0) {
            free(pa);
            printf("music nand %s buff failed: IO failed\n", err[rd]);
            return -5;
        }

        len -= iolen;
        buf += iolen;
        addr += iolen;
        *iodone += iolen;
    }

    free(pa);

    return 0;
}

static sa_i32_t music_nand_rw_oob(struct mtd_info *mtd, sa_i32_t rd, loff_t addr,
                  struct mtd_oob_ops *ops)
{
    sa_i32_t ret = 0;
    sa_u8_t *pa = NULL;
    sa_u32_t addr0 = 0, addr1 = 0;
    sa_u8_t *err[] = { "write", "read" };
    sa_u8_t *oob;

    pa = malloc(sizeof (sa_u8_t) * (mtd->oobblock+ mtd->oobsize));
    oob = pa + mtd->oobblock;

    if (NULL == pa) {
        printf("MUSIC NAND %s oob failed: memory exhausted \n", err[rd]);
        return -12;
    }
    music_nand_get_reg_addr(addr, &addr0, &addr1);

    if (!rd) {
        if (ops->datbuf) {
            /*
	                * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	                * We assume that the caller gives us a full
	                * page to write. We don't read the page and
	                * update the changed portions alone.
	                *
	                * Hence, not checking for len < or > pgsz etc...
	                * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	                */
		memcpy(pa, ops->datbuf, ops->len);
        }

        if (ops->mode == MTD_OOB_PLACE) {
            oob += ops->ooboffs;
        } else if (ops->mode == MTD_OOB_AUTO) {
            /*
			  * comments by luny, it seems no 0xff 0xff in the first two bytes
			  * in OOB region.
			  */
            // clean markers
            oob[0] = oob[1] = 0xff;
            oob += 2;
        }

        memcpy(oob, ops->oobbuf, ops->ooblen);
    }

	flush_cache(pa, mtd->oobblock + mtd->oobsize);	// for writes...
    ret = music_nand_rw_page(rd, addr0, addr1, mtd->oobblock + mtd->oobsize, virt_to_phys(pa));
	flush_cache(pa, mtd->oobblock + mtd->oobsize);	// for read

    if (ret != 0) {
        free(pa);
        return 1;
    }

    if (rd) {
        if (ops->datbuf) {
            memcpy(ops->datbuf, pa, ops->len);
        }

        if (ops->mode == MTD_OOB_PLACE) {
            oob += ops->ooboffs;
        } else if (ops->mode == MTD_OOB_AUTO) {
            /*
			  * comments by luny, it seems no 0xff 0xff in the first two bytes
			  * in OOB region.
			  */
            // copy after clean marker
            oob += 2;
        }

        memcpy(ops->oobbuf, oob, ops->ooblen);
    }

    if (ops->datbuf) {
        ops->retlen = ops->len;
    }
    ops->oobretlen = ops->ooblen;

    free(pa);
    return 0;

}


static sa_i32_t music_nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    uint64_t s_first = 0, s_last = 0;
    sa_u32_t addr = 0;
    sa_u32_t addr0 = 0, addr1 = 0;

    if (instr->addr + instr->len > mtd->size) {
        printf("MUSIC NAND erase failed: erase size too large \n");
        return (-22);
    }

    s_first = instr->addr;
    s_last = s_first + instr->len;

    for (addr = s_first; addr < s_last; addr += mtd->erasesize) {
        music_nand_get_reg_addr(addr, &addr0, &addr1);
        music_nand_block_erase(addr0, addr1);
    }

    if (instr->callback) {
        instr->state |= MTD_ERASE_DONE;
        instr->callback(instr);
    }

    return 0;
}

static sa_i32_t music_nand_read(struct mtd_info *mtd, loff_t from, sa_size_t len,
                sa_size_t * retlen, u_char * buf)
{
    sa_i32_t ret = 0;
    sa_i32_t rd = 1;

    if ((!len) || (!retlen) || (!buf)) {
        printf("MUSIC NAND read failed: empty pointer \n");
        return -22;
    }

    MUSIC_KDEBUG("MUSIC NAND read from: 0x%llx len: 0x%lx \n", from, len);

    ret = music_nand_rw_buff(mtd, rd, buf, from, len, retlen);
    return ret;
}


static sa_i32_t music_nand_write(struct mtd_info *mtd, loff_t to, sa_size_t len,
                 sa_size_t * retlen, const u_char * buf)
{
    sa_i32_t ret = 0;
    sa_i32_t rd = 0;

    if ((!len) || (!retlen) || (!buf)) {
        printf("MUSIC NAND write failed: empty pointer \n");
        return -22;
    }

    MUSIC_KDEBUG("MUSIC NAND write to: 0x%llx len: 0x%lx \n", to, len);
    printf("MUSIC NAND write to: 0x%llx len: 0x%lx \n", to, len);

    ret = music_nand_rw_buff(mtd, rd, (u_char *) buf, to, len, retlen);

    return ret;
}

static sa_i32_t music_nand_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	sa_i32_t rd = 1;
    sa_i32_t ret = 0;

	if (NULL == ops) {
        printf("MUSIC NAND read oob failed: empty point \n");
        return -22;
    }

    MUSIC_KDEBUG("MUSIC NAND read oob from: 0x%llx ops->len: 0x%x ops->ooblen: 0x%x datbuf: %p\n",
         from, ops->len, ops->ooblen, ops->datbuf);

    ret = music_nand_rw_oob(mtd, rd, from, ops);

	return ret;
}

static sa_i32_t music_nand_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
    sa_i32_t rd = 0;
    sa_i32_t ret = 0;
    unsigned char oob[128];
    struct mtd_oob_ops rops = {
        .mode = MTD_OOB_RAW,
        .ooblen = mtd->oobsize,
        .oobbuf = oob,
    };

    MUSIC_KDEBUG("%s: from: 0x%llx mode: 0x%x len: 0x%x retlen: 0x%x\n"
                 "ooblen: 0x%x oobretlen: 0x%x ooboffs: 0x%x datbuf: %p "
                 "oobbuf: %p\n", __func__, to,
                 ops->mode, ops->len, ops->retlen, ops->ooblen,
                 ops->oobretlen, ops->ooboffs, ops->datbuf, ops->oobbuf);

    if (ops->mode == MTD_OOB_AUTO) {
        /* read existing oob */
        ret = music_nand_read_oob(mtd, to, &rops);
        if (ret || rops.oobretlen != rops.ooblen) {
            printf("%s: oob read failed at 0x%llx\n", __func__, to);
            return 1;
        }
        /*
		  * comments by luny, it seems no 0xff 0xff in the first two bytes
		  * in OOB region.
		  */
        memcpy(oob + 2, ops->oobbuf, ops->ooblen);
        //memcpy(oob, ops->oobbuf, ops->ooblen);
        rops = *ops;
        ops->oobbuf = oob;
        ops->ooblen = mtd->oobsize;
        ops->mode = MTD_OOB_RAW;
    }

    if (NULL == ops) {
        printf("MUSIC NAND write oob failed: empty point \n");
        return -22;
    }

    MUSIC_KDEBUG("MUSIC NAND write oob to: 0x%llx ops->len: 0x%x ops->ooblen: 0x%x \n",
         to, ops->len, ops->ooblen);

    ret = music_nand_rw_oob(mtd, rd, to, ops);

    return ret;
}

static sa_i32_t music_nand_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
    sa_i32_t ret = 0;
    sa_u8_t *pa = NULL;
    sa_u32_t addr0 = 0, addr1 = 0;
    sa_u8_t *oob;

    pa = malloc(sizeof (sa_u8_t) * (mtd->oobblock + mtd->oobsize));
    oob = pa + mtd->oobblock;

    if (NULL == pa) {
        printf("MUSIC NAND read oob failed: memory exhausted \n");
        return -12;
    }
    music_nand_get_reg_addr(ofs, &addr0, &addr1);

    flush_cache(pa, mtd->oobblock + mtd->oobsize);	// for writes...
    ret = music_nand_rw_page(1, addr0, addr1, mtd->oobblock + mtd->oobsize, virt_to_phys(pa));
    flush_cache(pa, mtd->oobblock + mtd->oobsize);	// for read

    if (ret != 0) {
        free(pa);
        return 1;
    }

    if((*(oob+0) == 0xff) && (*(oob+1) == 0xff))
        ret = 0;
    else
    {
        ret = 1;
        //printf("badblock ofs:0x%x ret:%d\n", (unsigned int)ofs, ret);
    }
    free(pa);
    return ret;
}

static sa_i32_t music_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
#if 0
    sa_i32_t ret = 0;
    sa_u8_t *pa = NULL;
    sa_u32_t addr0 = 0, addr1 = 0;
    sa_u8_t *oob;

    pa = malloc(sizeof (sa_u8_t) * (mtd->oobblock + mtd->oobsize));
    oob = pa + mtd->oobblock;

    if (NULL == pa) {
        printf("MUSIC NAND read oob failed: memory exhausted \n");
        return -12;
    }
    music_nand_get_reg_addr(ofs, &addr0, &addr1);

    ret = music_nand_rw_page(1, addr0, addr1, mtd->oobblock + mtd->oobsize, virt_to_phys(pa));
    *(oob+0) = *(oob+1) = 0;
    ret = music_nand_rw_page(0, addr0, addr1, mtd->oobblock + mtd->oobsize, virt_to_phys(pa));

    if (ret != 0) {
        free(pa);
        return 1;
    }

    free(pa);
#endif
    return 0;
}
