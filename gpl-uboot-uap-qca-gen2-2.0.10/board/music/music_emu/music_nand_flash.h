#ifndef _NAND_API_H
#define _NAND_API_H

struct nand_api {
    void (*_nand_init)(void);
    void (*_nand_read)(void);
};

void
nand_module_install(struct nand_api *api);

#define MUSIC_NAND_FLASH_BASE	0xba000000u
#define MUSIC_NF_RST		(MUSIC_NAND_FLASH_BASE + 0x00u)
#define MUSIC_NF_CTRL		(MUSIC_NAND_FLASH_BASE + 0x04u)
#define MUSIC_NF_RST_REG	(MUSIC_NAND_FLASH_BASE + 0x08u)
#define MUSIC_NF_ADDR0_0	(MUSIC_NAND_FLASH_BASE + 0x1cu)
#define MUSIC_NF_ADDR0_1	(MUSIC_NAND_FLASH_BASE + 0x24u)
#define MUSIC_NF_DMA_ADDR	(MUSIC_NAND_FLASH_BASE + 0x64u)
#define MUSIC_NF_DMA_COUNT	(MUSIC_NAND_FLASH_BASE + 0x68u)
#define MUSIC_NF_DMA_CTRL	(MUSIC_NAND_FLASH_BASE + 0x6cu)
#define MUSIC_NF_MEM_CTRL	(MUSIC_NAND_FLASH_BASE + 0x80u)
#define MUSIC_NF_PG_SIZE	(MUSIC_NAND_FLASH_BASE + 0x84u)
#define MUSIC_NF_RD_STATUS	(MUSIC_NAND_FLASH_BASE + 0x88u)
#define MUSIC_NF_TIMINGS_ASYN	(MUSIC_NAND_FLASH_BASE + 0x90u)

#define MUSIC_NF_BLK_SIZE_S		0x11
#define MUSIC_NF_BLK_SIZE		(64*2048)  //(1 << MUSIC_NF_BLK_SIZE_S)  //Number of Pages per block; 0=32, 1=64, 2=128, 3=256
#define MUSIC_NF_BLK_SIZE_M		(MUSIC_NF_BLK_SIZE - 1)
//#define MUSIC_NF_PAGE_SIZE		2112	//No of bytes per page; 0=256, 1=512, 2=1024, 3=2048, 4=4096, 5=8182, 6= 16384, 7=0
#define MUSIC_NF_PAGE_SIZE		2048	//No of bytes per page; 0=256, 1=512, 2=1024, 3=2048, 4=4096, 5=8182, 6= 16384, 7=0
#define MUSIC_NF_CUSTOM_SIZE_EN	0x1	//1 = Enable, 0 = Disable
#define MUSIC_NF_ADDR_CYCLES_NUM	0x5	//No of Address Cycles
#define MUSIC_NF_TIMING_ASYN		0x0
#define MUSIC_NF_STATUS_OK		0xc0
#define MUSIC_NF_READ_STATUS_MASK	0xc7

typedef struct {
	u_int8_t	sa1	: 1,	// Serial access time (bit 1)
			org	: 1,	// Organisation
			bs	: 2,	// Block size
			sa0	: 1,	// Serial access time (bit 0)
			ss	: 1,	// Spare size per 512 bytes
			ps	: 2,	// Page Size

			wc	: 1,	// Write Cache
			ilp	: 1, 	// Interleaved Programming
			nsp	: 2, 	// No. of simult prog pages
			ct	: 2,	// Cell type
			dp	: 2,	// Die/Package

			did,		// Device id
			vid,		// Vendor id

			res1	: 2,	// Reserved
			pls	: 2,	// Plane size
			pn	: 2,	// Plane number
			res2	: 2;	// Reserved
} ath_nand_id_t;

typedef struct {
	ath_nand_id_t	id;
	u_int64_t	size;	 /* Total size of the MTD */
	u_int32_t	erasesize,
			erasesize_shift,
			erasesize_mask,
			writesize,
			oobsize;

} ath_nand_flinfo_t;


#endif
