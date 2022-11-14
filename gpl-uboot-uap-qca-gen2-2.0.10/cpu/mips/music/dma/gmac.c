#include "gmac.h"
#include <net.h>

typedef void (*handle_func)(volatile uchar * inpkt, int len);

struct protocol_handle {
    uint32_t    enable;
    uint32_t    number;
    handle_func handle;
    struct protocol_handle * next;
};

static uint8_t rx_buf_cp[ATHR_RX_BUF_SIZE] = {0};
struct protocol_handle * handle_header = NULL;

void
protocol_test_smac(volatile uchar * inpkt, int len)
{

    Ethernet_t *et = (Ethernet_t *)(inpkt);
	uchar  server_mac[6] = {0x00, 0x21, 0xCC, 0xBD, 0x77, 0xBD};

	if((memcmp(et->et_src, server_mac, 6)) == 0) {
		printf("callback check smac ");
		printf("YES!  got smac: 00:19:B9:E0:04:F6\n");
	}
}

void
protocol_test_prot(volatile uchar * inpkt, int len)
{
    Ethernet_t *et = (Ethernet_t *)(inpkt);

	if(ntohs(et->et_protlen) == PROT_ARP) {
		printf("callback check prot ");
		printf("YES!  got arp\n");
	}
}

void
protocol_handle_add(handle_func  handle)
{
    struct protocol_handle * search = handle_header;
    struct protocol_handle * last = handle_header;
    if(handle_header == NULL) {
        handle_header = malloc(sizeof(struct protocol_handle));
        handle_header->enable = 1;
        handle_header->next=NULL;
        handle_header->handle = handle;
        return;
    }
    search = handle_header->next;
    last = handle_header;

    while(search != NULL) {
        if(search->handle == handle) {
            search->enable = 1;
            return;
        }
        last = search;
        search = search->next;
    }

    search = malloc(sizeof(struct protocol_handle));
    if(search ==NULL) {
        printf("Add protocol handle failed: no memory\n");
        return;
    }
    last->next = search;
    search->enable = 1;
    search->next = NULL;
    search->handle = handle;

    return;
}

void
protocol_handle_process(volatile uchar * data, int size)
{
    struct protocol_handle * process = handle_header;
    while (process != NULL) {
        if(process->enable == 0) {
            process = process->next;
            continue;
        }
        if(process->handle == NULL) {
            process = process->next;
            continue;
        }

		if(size < ATHR_RX_BUF_SIZE) {
			memset(rx_buf_cp, 0, ATHR_RX_BUF_SIZE);
			memcpy(rx_buf_cp, data, size);
			process->handle(rx_buf_cp, size);
		}

        process = process->next;
    }
    return;
}

void
protocol_handle_init (void)
{
    protocol_handle_add(PingReply);
    //protocol_handle_add(protocol_test_smac);
	//protocol_handle_add(protocol_test_prot);
}

static uint8_t *desc_buf;
static gmac_tpd_t *tpd_desc;
static gmac_rfd_t *rfd_desc;
static gmac_rrd_t *rrd_desc;

static uint8_t *rx_buf;
static uint8_t rx_id, rx_next_id;

static void
gmac_desc_setup(void)
{
    uint32_t i;
    rx_buf = KSEG1ADDR((uint8_t *)malloc(ATHR_NUM_RX_DESC * (ATHR_RX_BUF_SIZE+DMA_ALIGN_SIZE)));
    desc_buf = KSEG1ADDR(((uint8_t *)malloc(DESC_BUF_SIZE)));
    memset(desc_buf, 0, sizeof(DESC_BUF_SIZE));

    /* TPD */
    tpd_desc = (gmac_tpd_t *)DMA_ALIGN_ADDR((uint32_t)desc_buf);
    gmac_reg_wr(REG_TX_BASE_ADDR_HI, 0);
    gmac_reg_wr(REG_NTPD_HEAD_ADDR_LO, PHYSADDR(tpd_desc));
    gmac_reg_wr(REG_TPD_RING_SIZE, (ATHR_NUM_TX_DESC & TPD_RING_SIZE_MASK));
    DMA_INFO("tpd_desc base 0x%08x\n", tpd_desc);

    /* RFD */
    rfd_desc = (gmac_rfd_t *)DMA_ALIGN_ADDR((uint32_t)tpd_desc +
                sizeof(gmac_tpd_t) * ATHR_NUM_TX_DESC);

    for(i=0; i < ATHR_NUM_RX_DESC; i++) {
        rfd_desc[i].buffer_addr = cpu_to_le32(PHYSADDR(DMA_ALIGN_ADDR(rx_buf[i])));
        rfd_desc[i].buffer_addr_hi = 0;
    }
    gmac_reg_wr(REG_RX_BASE_ADDR_HI, 0);
    gmac_reg_wr(REG_RFD0_HEAD_ADDR_LO, PHYSADDR(rfd_desc));
    gmac_reg_wr(REG_RFD_RING_SIZE, (ATHR_NUM_RX_DESC & RFD_RING_SIZE_MASK));
    gmac_reg_wr(REG_RX_BUF_SIZE, ATHR_RX_MTU & RX_BUF_SIZE_MASK);
    DMA_INFO("rfd_desc base 0x%08x\n", rfd_desc);

    /* RRD */
    rrd_desc = (gmac_rrd_t *)DMA_ALIGN_ADDR((uint32_t)rfd_desc +
                sizeof(gmac_rfd_t) * ATHR_NUM_RX_DESC);

    gmac_reg_wr(REG_RRD0_HEAD_ADDR_LO, PHYSADDR(rrd_desc));
    gmac_reg_wr(REG_RRD_RING_SIZE, (ATHR_NUM_RX_DESC & RRD_RING_SIZE_MASK));
    DMA_INFO("rrd_desc base 0x%08x\n", rrd_desc);

    /* Load all of base address above */
    gmac_reg_wr(REG_LOAD_PTR, 1);

    rx_id = 0;
    rx_next_id = 1;
    gmac_reg_wr(REG_MB_RFD0_PROD_IDX, (rx_next_id & MB_RFDX_PROD_IDX_MASK));

    return;
}

static void
gmac_tx_setup(void)
{
    uint32_t data = 0;

    gmac_reg_wr(REG_TX_TSO_OFFLOAD_THRESH, 1500);
    data = (ATL1C_TPD_BURST & TXQ_NUM_TPD_BURST_MASK) <<
                              TXQ_NUM_TPD_BURST_SHIFT;
    data |= TXQ_CTRL_ENH_MODE;
    data |= (128* (2^ATL1C_DMAR_BLOCK)& TXQ_TXF_BURST_NUM_MASK) <<
                                        TXQ_TXF_BURST_NUM_SHIFT;
    gmac_reg_wr(REG_TXQ_CTRL, data);

    return;
}

static void
gmac_rx_setup(void)
{
    uint32_t data = 0;

    data = (ATL1C_RFD_BURST & RXQ_RFD_BURST_NUM_MASK) <<
                                  RXQ_RFD_BURST_NUM_SHIFT;
    gmac_reg_wr(REG_RXQ_CTRL, data);

    return;
}

static void
gmac_dma_setup(void)
{
    uint32_t data = 0;

    data |= DMA_CTRL_DMAR_OUT_ORDER;
    data |= (((uint32_t)ATL1C_DMAR_BLOCK) & DMA_CTRL_DMAR_BURST_LEN_MASK)
        << DMA_CTRL_DMAR_BURST_LEN_SHIFT;
    data |= (((uint32_t)ATL1C_DMAW_BLOCK) & DMA_CTRL_DMAW_BURST_LEN_MASK)
        << DMA_CTRL_DMAW_BURST_LEN_SHIFT;
    data |= (((uint32_t)ATL1C_DMAR_DLY_CNT) & DMA_CTRL_DMAR_DLY_CNT_MASK)
        << DMA_CTRL_DMAR_DLY_CNT_SHIFT;
    data |= (((uint32_t)ATL1C_DMAW_DLY_CNT) & DMA_CTRL_DMAW_DLY_CNT_MASK)
        << DMA_CTRL_DMAW_DLY_CNT_SHIFT;

    gmac_reg_wr(REG_DMA_CTRL, data);

    return;
}

static void
gmac_mac_setup(void)
{
    uint32_t data = 0;

    /*mac reset*/
    data = gmac_reg_rd(REG_MASTER_CTRL);
    gmac_reg_wr(REG_MASTER_CTRL, (data | MASTER_CTRL_SOFT_RST));
    udelay(1000*1000);
    //wmb();

    /*atheros header disable*/
    data = gmac_reg_rd(REG_ATHR_HEADER_CTRL);
    data &= ~ATHR_HEADER_EN;
    data &= ~ATHR_HEADER_CNT_EN;
    gmac_reg_wr(REG_ATHR_HEADER_CTRL, data);

    /*big endian enable*/
    data = gmac_reg_rd(REG_AXI_MAST_CTRL);
    gmac_reg_wr(REG_AXI_MAST_CTRL, (data | AXI_MAST_ENDIAN_SWAP_EN));

    /*rfd/tpd/rrd setup*/
    gmac_desc_setup();

    /* MTU setup*/
    gmac_reg_wr(REG_MTU, ATHR_RX_MTU);

    gmac_tx_setup();
    gmac_rx_setup();
    gmac_dma_setup();

    /*mac setup*/
    data = 0;
    data |= (MAC_CTRL_TX_EN | MAC_CTRL_RX_EN);
    data |= (MAC_CTRL_TX_FLOW | MAC_CTRL_RX_FLOW);
    data |= ((ATL1C_MAC_SPEED_1000 & MAC_CTRL_SPEED_MASK) <<
                                     MAC_CTRL_SPEED_SHIFT);
    data |= MAC_CTRL_DUPLX;
    data |= (MAC_CTRL_ADD_CRC | MAC_CTRL_PAD);
    data |= ((ATL1C_PREAMBLE_LEN & MAC_CTRL_PRMLEN_MASK) <<
                                    MAC_CTRL_PRMLEN_SHIFT);
    data |= MAC_CTRL_BC_EN;
    data |= MAC_CTRL_PROMIS_EN;
    data |= MAC_CTRL_RX_CHKSUM_EN;
    data |= MAC_CTRL_SINGLE_PAUSE_EN;
    gmac_reg_wr(REG_MAC_CTRL, data);

    /*txq enable*/
    data = gmac_reg_rd(REG_TXQ_CTRL);
    gmac_reg_wr(REG_TXQ_CTRL, (data | TXQ_CTRL_EN));

    /*rxq enable*/
    data = gmac_reg_rd(REG_RXQ_CTRL);
    gmac_reg_wr(REG_RXQ_CTRL, (data | RXQ_CTRL_EN));

    DMA_INFO("gmac_mac_setup done\n");
}

static void
gmac_pkt_tx(gmac_txbuf_t *tb)
{
    uint32_t i = 0;
    uint32_t data = 0, hw_next_clean = 0;
    gmac_tpd_t *this_tpd = &tpd_desc[tb->tx_id];

    memset(this_tpd, 0, sizeof(gmac_tpd_t));

    this_tpd->buffer_addr = cpu_to_le32(PHYSADDR(tb->buf));
    this_tpd->word0 = cpu_to_le32(tb->datalen&0xffff);
    this_tpd->word1 = cpu_to_le32(1 << TPD_EOP_SHIFT);

    data = (tb->next_id & MB_PRIO_PROD_IDX_MASK) <<
                                MB_NTPD_PROD_IDX_SHIFT;
    gmac_reg_wr(REG_MB_PRIO_PROD_IDX, data);

    for (i = 0; i < 1000; i++) {
        data = gmac_reg_rd(REG_MB_PRIO_CONS_IDX);
        hw_next_clean = (data >> MB_NTPD_CONS_IDX_SHIFT) &
                                    MB_PRIO_CONS_IDX_MASK;

        if (hw_next_clean != tb->tx_id) {
            break;
        }

        udelay(1000);
    }

    if(i == 1000) {
        DMA_ERR("###gmac_pkt_tx timeout###\n");
    }
}

static uint8_t
gmac_is_rx_pkt(uint32_t *buf_addr, uint32_t *pkt_size)
{
    uint32_t rrs_word3 = le32_to_cpu(rrd_desc[rx_id].word3);

    if (RRS_RXD_IS_VALID(rrs_word3)) {
        *pkt_size = (rrs_word3 >> RRS_PKT_SIZE_SHIFT) & RRS_PKT_SIZE_MASK;
        *buf_addr = DMA_ALIGN_ADDR(rx_buf[rx_id]);
        return 1;
    }

    return 0;
}

static void
gmac_rx_clean(void)
{
    rrd_desc[rx_id].word3 &= ~(cpu_to_le32(1 << RRS_RXD_UPDATED_SHIFT));
    rx_id = (rx_id + 1) % ATHR_NUM_RX_DESC;
    rx_next_id = (rx_next_id + 1) % ATHR_NUM_RX_DESC;
    gmac_reg_wr(REG_MB_RFD0_PROD_IDX, (rx_next_id & MB_RFDX_PROD_IDX_MASK));
}

static uint32_t
gmac_rcv(struct eth_device *dev)
{
    uint32_t pkt_size = 0, buf_addr;

    if(gmac_is_rx_pkt(&buf_addr, &pkt_size)) {
        NetReceive((void *)buf_addr , pkt_size);
        gmac_rx_clean();
    }

    return 0;
}

static int gmac_get_ethaddr(unsigned char *mac, int unit)
{
    mac[0] = 0x00;
    mac[1] = 0x03;
    mac[2] = 0x7f;
    mac[3] = 0x09;
    mac[4] = 0x0b;
    mac[5] = 0xad;

    return 0;
}
static int gmac_send(struct eth_device *dev, volatile void *packet, int length)
{
    static uint32_t	tx_id = 0;
    gmac_txbuf_t tb;

    flush_cache((u32) packet, length);

    tb.buf = packet;
    tb.datalen = length;
    tb.tx_id = tx_id;
    tx_id = (tx_id + 1) % ATHR_NUM_TX_DESC;
    tb.next_id = tx_id;

    gmac_pkt_tx(&tb);
}

static int gmac_init(struct eth_device *dev, bd_t * bd)
{
    return(1);
}

static void gmac_halt(struct eth_device *dev)
{

}

#ifdef MUSIC_FPGA_BOARD
static void phy_reg_wr(uint32_t reg_addr, uint32_t phy_data)
{
    uint32_t i =100;

    music_reg_wr(0xb80a0000, phy_data);
    udelay(100);
    music_reg_wr(0xb80a0008, reg_addr);
    udelay(100);
    music_reg_wr(0xb80a0014, 0x10003);
    udelay(100);

    while(i--) {
        if((music_reg_rd(0x180a0024)&0x1) == 0)  {
            break;
        }
        udelay(100);
    }
}

static void phy_debug_reg_wr(uint32_t reg_addr, uint32_t phy_data)
{
    phy_reg_wr(0x1d, reg_addr);
    phy_reg_wr(0x1e, phy_data);
}

void switch_port_init(void)
{
    /*cfg 0x04*/
    phy_reg_wr(0x04, 0x0de1);

    /*cfg 0x09*/
    phy_reg_wr(0x09, 0x0);

    /*soft reset*/
    phy_reg_wr(0x00, 0x9000);
    udelay(1000);

    /*cfg debug 0*/
    phy_debug_reg_wr(0x0, 0x034e);

    /*cfg debug 5*/
    phy_debug_reg_wr(0x5, 0x3d47);

    //udelay(100*1000);
}

#else
#define REG_MDIO_CTRL  0x15004
#define MDIO_CTRL_MDIO_BUSY_MASK    	0x80000000
#define MDIO_CTRL_MDIO_BUSY_OFFSET  	31
#define MDIO_CTRL_PHY_SEL_MASK    		0x30000000
#define MDIO_CTRL_PHY_SEL_OFFSET    	28
#define MDIO_CTRL_MDIO_CMD_MASK	    	0x08000000
#define MDIO_CTRL_MDIO_CMD_OFFSET   	27
#define MDIO_CTRL_MDIO_NO_SUP_PRE_MASK	 0x04000000
#define MDIO_CTRL_MDIO_NO_SUP_PRE_OFFSET 26
#define MDIO_CTRL_PHY_ADDR_MASK    		0x03e00000
#define MDIO_CTRL_PHY_ADDR_OFFSET    	21
#define MDIO_CTRL_REG_ADDR_MASK    		0x001f0000
#define MDIO_CTRL_REG_ADDR_OFFSET  		16
#define MDIO_CTRL_MDIO_DATA_MASK    	0x0000ffff
#define MDIO_CTRL_MDIO_DATA_OFFSET    	0
#define MDIO_CMD_READ  1
#define MDIO_CMD_WRITE 0

#define SWITCH_REG_BASE 0xb8800000

#define switch_reg_rd(r)	({		\
	music_reg_rd(SWITCH_REG_BASE + (r));	\
})

#define switch_reg_wr(r, v)	({	\
	music_reg_wr((SWITCH_REG_BASE + (r)), v);	\
})

/*phy_id:0~3 port_id:0~7*/
static int switch_mdio_phy_rd(char *devname, unsigned char phaddr,
	       unsigned char reg, unsigned short *value)
{
	unsigned int data = 0, cnt = 10;

	data |=  (phaddr << MDIO_CTRL_PHY_ADDR_OFFSET)& MDIO_CTRL_PHY_ADDR_MASK;
	data |=  (reg << MDIO_CTRL_REG_ADDR_OFFSET) & MDIO_CTRL_REG_ADDR_MASK;
	data |=  (MDIO_CMD_READ<<MDIO_CTRL_MDIO_CMD_OFFSET)& MDIO_CTRL_MDIO_CMD_MASK;
	data |=  ((phaddr/8) << MDIO_CTRL_PHY_SEL_OFFSET) & MDIO_CTRL_PHY_SEL_MASK;
	data |=  (1 << MDIO_CTRL_MDIO_BUSY_OFFSET) & MDIO_CTRL_MDIO_BUSY_MASK;

	switch_reg_wr(REG_MDIO_CTRL, data);

    do {
		udelay(1000);
    } while((switch_reg_rd(REG_MDIO_CTRL) & MDIO_CTRL_MDIO_BUSY_MASK) && (cnt--));

	*value = ((switch_reg_rd(REG_MDIO_CTRL) & MDIO_CTRL_MDIO_DATA_MASK)>>
											MDIO_CTRL_MDIO_DATA_OFFSET);
    return 0;
}

int switch_mdio_phy_wr (char *devname, unsigned char phaddr,
	        unsigned char reg, unsigned short value)
{
	unsigned int data = 0, cnt = 10;

	data |=  (phaddr << MDIO_CTRL_PHY_ADDR_OFFSET)& MDIO_CTRL_PHY_ADDR_MASK;
	data |=  (reg << MDIO_CTRL_REG_ADDR_OFFSET) & MDIO_CTRL_REG_ADDR_MASK;
	data |=  (MDIO_CMD_WRITE<<MDIO_CTRL_MDIO_CMD_OFFSET) & MDIO_CTRL_MDIO_CMD_MASK;
	data |=  ((phaddr/8) << MDIO_CTRL_PHY_SEL_OFFSET) & MDIO_CTRL_PHY_SEL_MASK;
	data |=  (value << MDIO_CTRL_MDIO_DATA_OFFSET) & MDIO_CTRL_MDIO_DATA_MASK;
	data |=  (1 << MDIO_CTRL_MDIO_BUSY_OFFSET) & MDIO_CTRL_MDIO_BUSY_MASK;

	switch_reg_wr(REG_MDIO_CTRL, data);

    do {
		udelay(1000);
    } while((switch_reg_rd(REG_MDIO_CTRL) & MDIO_CTRL_MDIO_BUSY_MASK) && (cnt--));

	return 0;
}
#endif

int ethr_net_poll_init = 0;
int music_gmac_initialize(bd_t *bis)
{
    struct eth_device *dev;
    int i, num_macs = 1;


    for (i = 0;i < num_macs; i++) {
        if((dev = (struct eth_device *)malloc(sizeof(struct eth_device))) == NULL) {
            printf("atl1c:malloc eth_device failed\n");
        }
        memset(dev, 0, sizeof(struct eth_device));

        /*init dev*/
        sprintf(dev->name, "eth%d", i);
#if 0
        gmac_get_ethaddr(dev->enetaddr, i);
#else
		memcpy(dev->enetaddr, bis->bi_enetaddr, 6);
#endif
        dev->iobase = 0;
        dev->init = gmac_init;
        dev->halt = gmac_halt;
        dev->send = gmac_send;
        dev->recv = gmac_rcv;
        dev->priv = NULL;

        eth_register (dev);

#if (CONFIG_COMMANDS & CFG_CMD_MII)
        miiphy_register(dev->name, switch_mdio_phy_rd, switch_mdio_phy_wr);
#endif

#ifdef MUSIC_FPGA_BOARD
        switch_port_init();
#else
        switch_reg_wr(0x100, 0);
        //udelay(1000*1000);
#endif
        gmac_mac_setup();

#ifdef MUSIC_FPGA_BOARD

#else
        //udelay(1000*1000);
        switch_reg_wr(0x100, 0x7e);
        //udelay(1000*1000);
#endif


        printf("%s up\n",dev->name);
	}
    protocol_handle_init();
    printf("music_gmac_initialize done\n");
    ethr_net_poll_init = 1;

    return i;
}



uint32_t
eth_net_polling()
{
    uint32_t pkt_size = 0, buf_addr;
    if(!ethr_net_poll_init) {
        return 0;
    }

    if(gmac_is_rx_pkt(&buf_addr, &pkt_size)) {
        protocol_handle_process((void *)buf_addr , pkt_size);
        gmac_rx_clean();
    }

    return 0;
}
