
#ifndef _GMAC_H_
#define _GMAC_H_


#include <config.h>
#include <common.h>
#include <malloc.h>
#include <net.h>
#include <command.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include <music_soc.h>

/********************
 *config definition*
 ********************/



#define SW_OK       0
#define SW_FAIL     1

#if 1
#define DMA_INFO
#define DMA_WARN
#define DMA_ERR
#else
#define DMA_INFO printf
#define DMA_WARN printf
#define DMA_ERR printf
#endif

#ifndef roundup
#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#endif


/********************
 *image definition*
 ********************/
#define ETH_ALEN                6
//#define ATH_P_BOOT            0x12
#define ATH_P_CTRL              0x12
#define ATH_P_DATA              0x13
#define ETH_P_ATH               0x88bd

/*
 * XXX Pack 'em
 */
typedef struct {
	uint16_t	more_data;	/* Is there more data? */
	uint16_t    len;           /* Len this segment    */
	uint32_t	offset;        /* Offset in the file  */
} fwd_cmd_t;
/*
 * No enums across platforms
 */
#define FWD_DISC        0x1
#define FWD_DISC_ACK    0x2
#define FWD_IMG_RECV    0x3
#define FWD_IMAGE_ACK   0x4
#define FWD_SUCCESS     0x5
#define FWD_FAILED      0x6
#define FWD_FIN         0x7

typedef struct {
	uint32_t    rsp;       /* UNKNOWN/DISC/DISC_ACK/ACK/SUCCESS/FAIL*/
	uint32_t    offset;    /* rsp for this ofset  */
} fwd_rsp_t;

struct __ethhdr{
    uint8_t       dst[ETH_ALEN];/*destination eth addr */
    uint8_t       src[ETH_ALEN]; /*source ether addr*/
    uint16_t      etype;/*ether type*/
}__attribute__((packed));
/**
 * @brief this is will be in big endian format
 */
struct __athhdr{
#ifdef LITTLE_ENDIAN
    uint8_t   proto:6,
                res:2;
#else
    uint8_t   res:2,
                proto:6;
#endif
    uint8_t   res_lo;
    uint16_t  res_hi;
}__attribute__((packed));

typedef struct gmac_hdr{
    struct  __ethhdr   eth;
    struct  __athhdr   ath;
    uint16_t         align_pad;/*pad it for 4 byte boundary*/
}__attribute__((packed))  gmac_hdr_t;

typedef struct {
	uint8_t		*buf;
	uint8_t     *data;
	uint8_t     tx_id;
    uint8_t     next_id;
	uint16_t	datalen;
} gmac_txbuf_t;

/* Sufficient for our discover and ack packets */
#define ATHR_TX_BUF_SIZE		64
/*
 * We receive:	hdr		  20
 *		fwd cmd		   8
 *		Load addr	   4	(Only for 1st packet)
 *		Data		1024
 *		Checksum	   4
 *				----
 *				1060	(Maximum)
 */
//#define ATHR_RX_BUF_SIZE	    1096
#define ATHR_RX_BUF_SIZE	    2000

#define DMA_ALIGN_SIZE		    8
#define DMA_ALIGN_ADDR(addr)	(roundup((uint32_t)(addr), DMA_ALIGN_SIZE))
#define DESC_BUF_SIZE (sizeof(gmac_tpd_t) * ATHR_NUM_TX_DESC + \
                       sizeof(gmac_rfd_t) * ATHR_NUM_RX_DESC + \
                       sizeof(gmac_rrd_t) * ATHR_NUM_RX_DESC + 8*3)


/********************
 *atl1c definition*
 ********************/



/* DMA Order Settings */
#define ATL1C_DMA_ORD_IN   1
#define ATL1C_DMA_ORD_ENH  2
#define ATL1C_DMA_ORD_OUT  4

#define ATL1C_DMA_REQ_128   0
#define ATL1C_DMA_REQ_256   1
#define ATL1C_DMA_REQ_512   2
#define ATL1C_DMA_REQ_1024  3
#define ATL1C_DMA_REQ_2048  4
#define ATL1C_DMA_REQ_4096  5

/*Configurable*/
#define ATL1C_PREAMBLE_LEN  7
#define ATL1C_TPD_BURST     5
#define ATL1C_RFD_BURST     8
#define ATL1C_DMAR_BLOCK    ATL1C_DMA_REQ_4096
#define ATL1C_DMAW_BLOCK    ATL1C_DMA_REQ_128
#define ATL1C_DMAR_DLY_CNT  15
#define ATL1C_DMAW_DLY_CNT  4
#define ATHR_RX_MTU	            1536
//#define ATL1C_DMA_ORDER     ATL1C_DMA_ORD_OUT



/*TPD desciptor*/
#define TPD_EOP_SHIFT	31
typedef struct gmac_tpd {
    uint32_t    word0;
	uint32_t	word1;
	uint32_t	buffer_addr;
	uint32_t	word3;
} gmac_tpd_t;

/*RRD desciptor*/
#define RRS_PKT_SIZE_MASK	0x3FFF
#define RRS_PKT_SIZE_SHIFT	0

#define RRS_RXD_UPDATED_MASK	0x0001
#define RRS_RXD_UPDATED_SHIFT	31
#define RRS_RXD_IS_VALID(word) \
	((((word) >> RRS_RXD_UPDATED_SHIFT) & RRS_RXD_UPDATED_MASK) == 1)

typedef struct gmac_rrd {
	uint32_t    word0;
    uint32_t	word1_svlan;
	uint32_t	word2_cvlan;
	uint32_t	word3;
} gmac_rrd_t;

/* RFD desciptor */
typedef struct gmac_rfd {
	uint32_t	buffer_addr;
    uint32_t	buffer_addr_hi;
} gmac_rfd_t;


#define ATHR_NUM_TX_DESC		2
#define ATHR_NUM_RX_DESC		2
#define ATL1C_MAC_SPEED_1000    2


#define GMAC_REG_BASE 0xb8110000

#define gmac_reg_rd(r)	({		\
	music_reg_rd(GMAC_REG_BASE + (r));	\
})

#define gmac_reg_wr(r, v)	({	\
	music_reg_wr((GMAC_REG_BASE + (r)), v);	\
})

#define SWITCH_REG_BASE 0xb8800000

#define switch_reg_rd(r)	({		\
	music_reg_rd(SWITCH_REG_BASE + (r));	\
})

#define switch_reg_wr(r, v)	({	\
	music_reg_wr((SWITCH_REG_BASE + (r)), v);	\
})

/* register definition */

/* Selene Master Control Register */
#define REG_MASTER_CTRL			0x1400
#define MASTER_CTRL_SOFT_RST        0x1
#define MASTER_CTRL_TEST_MODE_MASK	0x3
#define MASTER_CTRL_TEST_MODE_SHIFT	2
#define MASTER_CTRL_BERT_START		0x10
#define MASTER_CTRL_OOB_DIS_OFF		0x40
#define MASTER_CTRL_SA_TIMER_EN		0x80
#define MASTER_CTRL_MTIMER_EN       0x100
#define MASTER_CTRL_MANUAL_INT      0x200
#define MASTER_CTRL_TX_ITIMER_EN	0x400
#define MASTER_CTRL_RX_ITIMER_EN	0x800
#define MASTER_CTRL_CLK_SEL_DIS		0x1000
#define MASTER_CTRL_CLK_SWH_MODE	0x2000
#define MASTER_CTRL_INT_RDCLR		0x4000
#define MASTER_CTRL_REV_NUM_SHIFT	16
#define MASTER_CTRL_REV_NUM_MASK	0xff
#define MASTER_CTRL_DEV_ID_SHIFT	24
#define MASTER_CTRL_DEV_ID_MASK		0x7f
#define MASTER_CTRL_OTP_SEL		    0x80000000


/* MAC Control Register  */
#define REG_MAC_CTRL        	0x1480
#define MAC_CTRL_TX_EN			        0x1
#define MAC_CTRL_RX_EN			        0x2
#define MAC_CTRL_TX_FLOW		        0x4
#define MAC_CTRL_RX_FLOW         	    0x8
#define MAC_CTRL_LOOPBACK          	    0x10
#define MAC_CTRL_DUPLX              	0x20
#define MAC_CTRL_ADD_CRC            	0x40
#define MAC_CTRL_PAD                	0x80
#define MAC_CTRL_LENCHK             	0x100
#define MAC_CTRL_HUGE_EN            	0x200
#define MAC_CTRL_PRMLEN_SHIFT       	10
#define MAC_CTRL_PRMLEN_MASK        	0xf
#define MAC_CTRL_RMV_VLAN           	0x4000
#define MAC_CTRL_PROMIS_EN          	0x8000
#define MAC_CTRL_TX_PAUSE           	0x10000
#define MAC_CTRL_SCNT               	0x20000
#define MAC_CTRL_SRST_TX            	0x40000
#define MAC_CTRL_TX_SIMURST         	0x80000
#define MAC_CTRL_SPEED_SHIFT        	20
#define MAC_CTRL_SPEED_MASK         	0x3
#define MAC_CTRL_DBG_TX_BKPRESURE   	0x400000
#define MAC_CTRL_TX_HUGE            	0x800000
#define MAC_CTRL_RX_CHKSUM_EN       	0x1000000
#define MAC_CTRL_MC_ALL_EN          	0x2000000
#define MAC_CTRL_BC_EN              	0x4000000
#define MAC_CTRL_DBG                	0x8000000
#define MAC_CTRL_SINGLE_PAUSE_EN	    0x10000000
#define MAC_CTRL_HASH_ALG_CRC32		    0x20000000
#define MAC_CTRL_SPEED_MODE_SW		    0x40000000


/* MAC STATION ADDRESS  */
#define REG_MAC_STA_ADDR		        0x1488


/* Maximum Frame Length Control Register   */
#define REG_MTU                     	0x149c

/*
 * Load Ptr Register
 * Software sets this bit after the initialization of the head and tail */
#define REG_LOAD_PTR                	0x1534

/*
 * addresses of all descriptors, as well as the following descriptor
 * control register, which triggers each function block to load the head
 * pointer to prepare for the operation. This bit is then self-cleared
 * after one cycle.
 */
#define REG_RX_BASE_ADDR_HI		0x1540
#define REG_TX_BASE_ADDR_HI		0x1544
#define REG_SMB_BASE_ADDR_HI		0x1548
#define REG_SMB_BASE_ADDR_LO		0x154C
#define REG_RFD0_HEAD_ADDR_LO		0x1550
#define REG_RFD1_HEAD_ADDR_LO		0x1554
#define REG_RFD2_HEAD_ADDR_LO		0x1558
#define REG_RFD3_HEAD_ADDR_LO		0x155C
#define REG_RFD_RING_SIZE		0x1560
#define RFD_RING_SIZE_MASK		0x0FFF
#define REG_RX_BUF_SIZE			0x1564
#define RX_BUF_SIZE_MASK		0xFFFF
#define REG_RRD0_HEAD_ADDR_LO		0x1568
#define REG_RRD1_HEAD_ADDR_LO		0x156C
#define REG_RRD2_HEAD_ADDR_LO		0x1570
#define REG_RRD3_HEAD_ADDR_LO		0x1574
#define REG_RRD_RING_SIZE		0x1578
#define RRD_RING_SIZE_MASK		0x0FFF
#define REG_HTPD_HEAD_ADDR_LO		0x157C
#define REG_NTPD_HEAD_ADDR_LO		0x1580
#define REG_TPD_RING_SIZE		0x1584
#define TPD_RING_SIZE_MASK		0xFFFF
#define REG_CMB_BASE_ADDR_LO		0x1588

/* TXQ Control Register */
#define REG_TXQ_CTRL                	0x1590
#define	TXQ_NUM_TPD_BURST_MASK     	0xF
#define TXQ_NUM_TPD_BURST_SHIFT    	0
#define TXQ_CTRL_IP_OPTION_EN		0x10
#define TXQ_CTRL_EN                     0x20
#define TXQ_CTRL_ENH_MODE               0x40
#define TXQ_CTRL_LS_8023_EN		0x80
#define TXQ_TXF_BURST_NUM_SHIFT    	16
#define TXQ_TXF_BURST_NUM_MASK     	0xFFFF

/* Jumbo packet Threshold for task offload */
#define REG_TX_TSO_OFFLOAD_THRESH	0x1594 /* In 8-bytes */
#define TX_TSO_OFFLOAD_THRESH_MASK	0x07FF

#define	REG_TXF_WATER_MARK		0x1598 /* In 8-bytes */
#define TXF_WATER_MARK_MASK		0x0FFF
#define TXF_LOW_WATER_MARK_SHIFT	0
#define TXF_HIGH_WATER_MARK_SHIFT 	16
#define TXQ_CTRL_BURST_MODE_EN		0x80000000

#define REG_THRUPUT_MON_CTRL		0x159C
#define THRUPUT_MON_RATE_MASK		0x3
#define THRUPUT_MON_RATE_SHIFT		0
#define THRUPUT_MON_EN			0x80

/* RXQ Control Register */
#define REG_RXQ_CTRL                	0x15A0
#define ASPM_THRUPUT_LIMIT_MASK		0x3
#define ASPM_THRUPUT_LIMIT_SHIFT	0
#define ASPM_THRUPUT_LIMIT_NO		0x00
#define ASPM_THRUPUT_LIMIT_1M		0x01
#define ASPM_THRUPUT_LIMIT_10M		0x02
#define ASPM_THRUPUT_LIMIT_100M		0x04
#define RXQ1_CTRL_EN			0x10
#define RXQ2_CTRL_EN			0x20
#define RXQ3_CTRL_EN			0x40
#define IPV6_CHKSUM_CTRL_EN		0x80
#define RSS_HASH_BITS_MASK		0x00FF
#define RSS_HASH_BITS_SHIFT		8
#define RSS_HASH_IPV4			0x10000
#define RSS_HASH_IPV4_TCP		0x20000
#define RSS_HASH_IPV6			0x40000
#define RSS_HASH_IPV6_TCP		0x80000
#define RXQ_RFD_BURST_NUM_MASK		0x003F
#define RXQ_RFD_BURST_NUM_SHIFT		20
#define RSS_MODE_MASK			0x0003
#define RSS_MODE_SHIFT			26
#define RSS_NIP_QUEUE_SEL_MASK		0x1
#define RSS_NIP_QUEUE_SEL_SHIFT		28
#define RRS_HASH_CTRL_EN		0x20000000
#define RX_CUT_THRU_EN			0x40000000
#define RXQ_CTRL_EN			0x80000000

#define REG_RFD_FREE_THRESH		0x15A4
#define RFD_FREE_THRESH_MASK		0x003F
#define RFD_FREE_HI_THRESH_SHIFT	0
#define RFD_FREE_LO_THRESH_SHIFT	6

/* DMA Engine Control Register */
#define REG_DMA_CTRL                	0x15C0
#define DMA_CTRL_DMAR_IN_ORDER          0x1
#define DMA_CTRL_DMAR_ENH_ORDER         0x2
#define DMA_CTRL_DMAR_OUT_ORDER         0x4
#define DMA_CTRL_RCB_VALUE              0x8
#define DMA_CTRL_DMAR_BURST_LEN_MASK    0x0007
#define DMA_CTRL_DMAR_BURST_LEN_SHIFT   4
#define DMA_CTRL_DMAW_BURST_LEN_MASK    0x0007
#define DMA_CTRL_DMAW_BURST_LEN_SHIFT   7
#define DMA_CTRL_DMAR_REQ_PRI           0x400
#define DMA_CTRL_DMAR_DLY_CNT_MASK      0x001F
#define DMA_CTRL_DMAR_DLY_CNT_SHIFT     11
#define DMA_CTRL_DMAW_DLY_CNT_MASK      0x000F
#define DMA_CTRL_DMAW_DLY_CNT_SHIFT     16
#define DMA_CTRL_CMB_EN               	0x100000
#define DMA_CTRL_SMB_EN			0x200000
#define DMA_CTRL_CMB_NOW		0x400000
#define MAC_CTRL_SMB_DIS		0x1000000
#define DMA_CTRL_SMB_NOW		0x80000000

/* Mail box */
#define MB_RFDX_PROD_IDX_MASK		0xFFFF
#define REG_MB_RFD0_PROD_IDX		0x15E0
#define REG_MB_RFD1_PROD_IDX		0x15E4
#define REG_MB_RFD2_PROD_IDX		0x15E8
#define REG_MB_RFD3_PROD_IDX		0x15EC

#define MB_PRIO_PROD_IDX_MASK		0xFFFF
#define REG_MB_PRIO_PROD_IDX		0x15F0
#define MB_HTPD_PROD_IDX_SHIFT		0
#define MB_NTPD_PROD_IDX_SHIFT		16

#define MB_PRIO_CONS_IDX_MASK		0xFFFF
#define REG_MB_PRIO_CONS_IDX		0x15F4
#define MB_HTPD_CONS_IDX_SHIFT		0
#define MB_NTPD_CONS_IDX_SHIFT		16

#define REG_MB_RFD01_CONS_IDX		0x15F8
#define MB_RFD0_CONS_IDX_MASK		0x0000FFFF
#define MB_RFD1_CONS_IDX_MASK		0xFFFF0000
#define REG_MB_RFD23_CONS_IDX		0x15FC
#define MB_RFD2_CONS_IDX_MASK		0x0000FFFF
#define MB_RFD3_CONS_IDX_MASK		0xFFFF0000

/*Endian Swap*/
#define REG_AXI_MAST_CTRL    		0x1610
#define AXI_MAST_ENDIAN_SWAP_EN     0x0008

/*Atheros Header*/
#define REG_ATHR_HEADER_CTRL		0x1620
#define ATHR_HEADER_EN		        0x0001
#define ATHR_HEADER_CNT_EN		    0x0002
extern uint32_t eth_net_polling(void);
#endif /*_GMAC_API_H_*/
