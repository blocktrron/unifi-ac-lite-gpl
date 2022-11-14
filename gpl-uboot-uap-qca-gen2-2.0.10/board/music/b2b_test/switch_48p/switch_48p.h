#ifndef _SWITCH_48P_H__
#define _SWITCH_48P_H__

#include <config.h>
#include <common.h>
#include <malloc.h>
#include <net.h>
#include <command.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include <music_soc.h>

#define _SWITCH_REG_FIELD_RELATIVE_MASK(len) ((1U<<len)-1)
#define _SWITCH_REG_FIELD_ABSOLUTE_MASK(offset, len)  (_SWITCH_REG_FIELD_RELATIVE_MASK(len)<<offset)
#define SWITCH_REG_FIELD_SET(reg_val, field_val, offset, len) \
    do{ \
        reg_val &= ~_SWITCH_REG_FIELD_ABSOLUTE_MASK(offset, len); \
        reg_val |= (field_val&_SWITCH_REG_FIELD_RELATIVE_MASK(len))<<offset; \
    }while(0)

#define PORT_MISC_CFG 0x2100
#define PORT_MISC_CFG_INC    0x10
#define IN_PORT_VLAN_TYPE    (1<<1)
#define PORT_JUMBO_EN        (1<<0)

#define PORT_EG_VLAN_CTRL 0x4600
#define PORT_EG_VLAN_CTRL_INC  0x10
#define PORT_VLAN_TYPE    (1<<4)

#define PORT_LRN_CTRL_1 0x6408
#define PORT_LRN_CTRL_1_INC  0x10
#define PORT_MAC_VLAN_TYPE (1<<10)

#define EG_PORT_TAG_CTRL 0x2500
#define EG_PORT_TAG_CTRL_INC  0x10
#define PORT_AHP_TAG_EN      (1<<7)
#define PORT_AHP_TPID_EN     (1<<6)
#define PORT_EG_DATA_TAG_EN  (1<<5)
#define PORT_EG_MGMT_TAG_EN  (1<<4)

#define PORT_STATUS_CONFIG 0x100
#define PORT_STATUS_CONFIG_INC  0x100
#define AUTO_FLOW_CTRL_EN  (1<<12)
#define AUTO_RX_FLOW_EN    (1<<11)
#define AUTO_TX_FLOW_EN    (1<<10)
#define AUTO_LINK_EN       (1<<9)
#define TX_HALF_FLOW_EN    (1<<7)
#define RX_FLOW_EN         (1<<5)
#define TX_FLOW_EN         (1<<4)
#define RXMAC_EN           (1<<3)
#define TXMAC_EN           (1<<2)

#define MUSIC_STACKING_UNICAST_FWD 0x2d0000
#define MUSIC_STACKING_UNICAST_FWD_INC 0x10

#define MUSIC_STACKING_MUL_FILT 0x2c0000
#define MUSIC_STACKING_MUL_FILT_INC 0x10


#define STK_TRUNK_MEM_0 0x6200
#define STK_TRUNK_MEM_0_INC  0x10
#define STK_TRUNK_MEM3_OFFSET 24
#define STK_TRUNK_MEM2_OFFSET 16
#define STK_TRUNK_MEM1_OFFSET 8
#define STK_TRUNK_MEM0_OFFSET 0

#define STK_TRUNK_MEM_1 0x6204
#define STK_TRUNK_MEM_1_INC  0x10
#define STK_TRUNK_MEM7_OFFSET 24
#define STK_TRUNK_MEM6_OFFSET 16
#define STK_TRUNK_MEM5_OFFSET 8
#define STK_TRUNK_MEM4_OFFSET 0

#define XGMAC_PORT_CONTROL 0x11008
#define XGMAC_PORT_CONTROL_INC  0x200
#define PROMIS_EN (1<<4)
#define RX_ENA    (1<<1)
#define TX_ENA    (1<<0)

#define GLOBAL_CTRL_1 0x8
#define MAC29SG_1P25G_EN  (1<<21)
#define MAC29SG_3P125G_EN (1<<20)
#define MAC28SG_1P25G_EN  (1<<19)
#define MAC28SG_3P125G_EN (1<<18)
#define MAC27SG_1P25G_EN  (1<<17)
#define MAC27SG_3P125G_EN (1<<16)
#define MAC26SG_1P25G_EN  (1<<15)
#define MAC26SG_3P125G_EN (1<<14)
#define MAC26XG_3P125G_EN (1<<13)
#define MAC26XG_4P3G_EN   (1<<12)
#define MAC25XG_3P125G_EN (1<<11)
#define MAC25XG_4P3G_EN   (1<<10)


#define XAUI_SERDES9_CTRL0 0x13414
#define XAUI_SERDES9_EN_SD_OFFSET 21


#define XAUI_SERDES9_CTRL1 0x13418
#define XAUI_SERDES9_BIT19 (1<<19)
#define XAUI_SERDES9_BIT20 (1<<20)
#define XAUI_SERDES9_EN_RX_OFFSET 6
#define XAUI_SERDES9_EN_TX_OFFSET 10

#define XAUI_SERDES13_CTRL0 0x1341c
#define XAUI_SERDES13_EN_SD_OFFSET 21

#define XAUI_SERDES13_CTRL1 0x13420
#define XAUI_SERDES13_BIT19 (1<<19)
#define XAUI_SERDES13_BIT20 (1<<20)
#define XAUI_SERDES13_EN_RX_OFFSET 6
#define XAUI_SERDES13_EN_TX_OFFSET 10



#define SD_CLK_SEL 0x20

#define GOL_FLOW_THD 0x10008
#define GOL_FLOW_THD_GOL_XON_THRES_OFFSET    16
#define GOL_FLOW_THD_GOL_XON_THRES_LEN    12
#define GOL_FLOW_THD_GOL_XOFF_THRES_OFFSET    0
#define GOL_FLOW_THD_GOL_XOFF_THRES_LEN    12

#define PORT_FLOW_THD 0x10010
#define PORT_FLOW_THD_INC  0x10
#define PORT_FLOW_THD_PORT_XON_THRES_OFFSET    16
#define PORT_FLOW_THD_PORT_XON_THRES_LEN    12
#define PORT_FLOW_THD_PORT_XOFF_THRES_OFFSET    0
#define PORT_FLOW_THD_PORT_XOFF_THRES_LEN    12


#define DEV_ID 0x4c
#define DEV_ID_DEV_ID_OFFSET    0
#define DEV_ID_DEV_ID_LEN    6


#define PORT_DYN_ALLOC_0 0xf400
#define PORT_DYN_ALLOC_0_INC  0x40
#define PORT_DYN_ALLOC_0_PORT_WRED_EN    (1<<31)

#define PORT_DYN_ALLOC_1 0xf404
#define PORT_DYN_ALLOC_1_INC  0x40

#define PORT_DYN_ALLOC_2 0xf408
#define PORT_DYN_ALLOC_2_INC  0x40

#define PORT_PRE_ALLOC_0 0xf100
#define PORT_PRE_ALLOC_0_INC  0x10

#define SWITCH_PORT_CFG_START 1
#define SWITCH_26PORT_UPLINK_PORT_ID    26
#define SWITCH_27PORT_UPLINK_PORT_ID    27

#define MUSIC_SWITCH_BASE 0x18800000
#define music_switch_wr(addr, val)  music_reg_wr(MUSIC_SWITCH_BASE + addr, val)
#define music_switch_rd(addr)       music_reg_rd(MUSIC_SWITCH_BASE + addr)

uint32_t
mdio_switch_reg_rd(uint32_t addr);
int32_t
mdio_switch_reg_wr(uint32_t addr, uint32_t data);

#endif
