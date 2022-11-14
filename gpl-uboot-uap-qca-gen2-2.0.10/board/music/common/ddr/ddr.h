#ifndef __MUSIC_DDR_H__
#define __MUSIC_DDR_H__


#include <common.h>

#include "music_soc.h"

#define DDR_DEBUG(fmt, msg...)
//#define DDR_DEBUG printf

#define CMD_NOP                      0x0 /* No operation */
#define CMD_LOADMODE                 0x1 /* execute command */
#define CMD_REFRESH                  0x3 /* Issued a auto fresh to sdram */
#define CMD_PRECHARGE_ALL            0x5 /* Deactived a open row in
                                                 * al sdram bank */
#define CMD_ACTIVATE                 0x6
#define CMD_WRITE                    0x8
#define CMD_READ                     0xa
#define CMD_ZQCS                     0xc
#define CMD_ZQCL                     0xd


#define DDR2_EMR1VAL0                0x0000
#define DDR2_EMR1VAL1                0x0380
#define DDR2_EMR1VAL2                0x0000
#define DDR2_EMR2VAL                 0x0000
#define DDR2_EMR3VAL                 0x0000

#define DDR3_EMR2VAL                 0x0008
#define DDR3_EMR3VAL                 0x0000
#define DDR3_EMR1VAL0                0x0000


#define CCLK_PERIOD_PER_USEC         0x200

#define DDR2_400_WR_VAL              3
#define DDR2_533_WR_VAL              4
#define DDR2_667_WR_VAL              5
#define DDR2_800_WR_VAL              6

#define DDR2_CL_256MB_VAL            3
#define DDR2_CL_512MB_VAL            3
#define DDR2_CL_1024MB_VAL           4
#define DDR2_CL_2048MB_VAL           6

#define DDR2_BURSTLENGTH_4           2
#define DDR2_BURSTLENGTH_8           3

#define DDR_BURSTTYPE_SEQUENTIAL     0
#define DDR_BURSTTYPE_INTERLEAVED    1


#define DDR_ALN_ONE_LANE     0
#define DDR_ALN_TWO_LANE     1
#define DDR_ALN_FOUR_LANE    3


#define DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM 1
#define DEFAULT_DQS_OUT_GATE_PHASE_LANE_NUM 1

#define DEFAULT_DQS_IN_EDLY_CODE_OFF   0x18
#define DEFAULT_DQS_IN_RDLY_CODE_OFF   0xc
#define DEFAULT_DQS_IN_FDLY_CODE_OFF   0xc


enum edge_type{
    RDLY_EDGE = 0,
    FDLY_EDGE = 1,
    EDLY_EDGE = 2
};

enum athr_ddr_density_e{
    DDR_64MB = 0,
    DDR_128MB = 1,
    DDR_256MB = 2,
    DDR_512MB = 3,
    DDR_1GB = 4,
    DDR_2GB = 5,
    DDR_4GB = 6,
    DDR_8GB = 7,
};

enum athr_ddr_mode_e{
    DDR1_MODE = 0,
    DDR2_MODE = 1,
    SDR_DRAM_MODE = 2,
    DDR3_MODE  = 3,
};


struct athr_ddr_init_config
{
    uint32_t burst_length;
    uint32_t burst_type;
    uint32_t CAS_latency;
    uint32_t write_recovery;
    uint32_t density;
    uint32_t lane_number;
    uint32_t ddr_mode;
    uint32_t addr_map_mode;
    uint32_t trrd;
    uint32_t trp;
    uint32_t trc;
    uint32_t trcd;
    uint32_t tras;
    uint32_t twtr;
    uint32_t trtw;
    uint32_t tccd;
    uint32_t trtp;
    uint32_t trfc;
    uint32_t tmod;
    uint32_t tmrd;
    uint32_t odt;
    uint32_t clock;
};


#define DDR2_800M_TWTR          5
#define DDR2_800M_TCCD          1
#define DDR2_800M_TRAS          0x14
#define DDR2_800M_TRTW          1
#define DDR2_800M_TRTP          4

enum athr_ddr_speed_mode_e {
    DDR_100M = 0,
    DDR_133M = 1,
    DDR_200M = 2,
    DDR_266M = 3,
    DDR_300M = 4,
    DDR_400M = 5,
    DDR_480M = 6,
};


#define DDR_DT_QDS_MAXNUM               4
#define DDR_DT_DATA_MAXNUM              4
#define DDR_DT_DLY_MAXNUM               0x40
#define DDR_I_SERDES_FLUSH_TIME         0x100


#define GLOBAL_BASE                     0x18000000
#define GLB_CLOCK_CTRL_REG              0x18000004
#define DDR_SPEED_MODE_SHIFT            11
#define DDR_SPEED_MODE_MASK             (0x7 << 11)


#define DDR2_BASEADDR                   0x18100000

#define DDR_CTRL_MISC_REG                      (DDR2_BASEADDR + 0x0)
#define MMU_L0_ARB_REG                         (DDR2_BASEADDR + 0X4)
#define MMU_L1_ARB0_REG                        (DDR2_BASEADDR + 0x8)
#define MMU_L1_ARB1_REG                        (DDR2_BASEADDR + 0XC)
#define MMU_L2_ARB0_REG                        (DDR2_BASEADDR + 0X10)
#define MMU_PRI_REG                            (DDR2_BASEADDR + 0x14)
#define DDR_CONFIG0_REG                        (DDR2_BASEADDR + 0X18)
#define DDR_CONFIG1_REG                        (DDR2_BASEADDR + 0X1c)
#define DDR_CONFIG2_REG                        (DDR2_BASEADDR + 0X20)
#define DDR_MODE_REGISTER_REG                  (DDR2_BASEADDR + 0X24)
#define DDR_MODE_REG                           (DDR2_BASEADDR + 0X24)
#define DDR_EXTENDED_MODE_REGISTER1_REG        (DDR2_BASEADDR + 0X28)
#define DDR_EXTENDED_MODE_REG1                 (DDR2_BASEADDR + 0X28)
#define DDR_EXTENDED_MODE_REGISTER2_REG        (DDR2_BASEADDR + 0X2c)
#define DDR_EXTENDED_MODE_REG2                 (DDR2_BASEADDR + 0X2c)
#define DDR_EXTENDED_MODE_REGISTER3_REG        (DDR2_BASEADDR + 0X30)
#define DDR_EXTENDED_MODE_REG3                 (DDR2_BASEADDR + 0X30)
#define DDR_REFRESH_REG                        (DDR2_BASEADDR + 0X34)
#define DDR_DDR2_CONFIG_REG                    (DDR2_BASEADDR + 0X38)
#define DDR_CFG_REG                            (DDR2_BASEADDR + 0X3c)
#define DDR_DT_ADDR_REG                        (DDR2_BASEADDR + 0x40)
#define DDR_DT0_REG                            (DDR2_BASEADDR + 0x44)
#define DDR_DT1_REG                            (DDR2_BASEADDR + 0x48)
#define DDR_DT2_REG                            (DDR2_BASEADDR + 0x4c)
#define DDR_DT3_REG                            (DDR2_BASEADDR + 0x50)
#define DDR_DT4_REG                            (DDR2_BASEADDR + 0x54)
#define DDR_DT5_REG                            (DDR2_BASEADDR + 0x58)
#define DDR_DT6_REG                            (DDR2_BASEADDR + 0x5c)
#define DDR_DT7_REG                            (DDR2_BASEADDR + 0x60)
#define DDR_TIMER_REG                          (DDR2_BASEADDR + 0x64)
#define DDR_MIN_REG                            (DDR2_BASEADDR + 0x68)
#define DDR_MAX_REG                            (DDR2_BASEADDR + 0x6c)
#define DDR_EXCEPTION_REG                      (DDR2_BASEADDR + 0x70)
#define DDR_PHY_MISC_REG                       (DDR2_BASEADDR + 0x74)
#define DLL_CTRL_REG                           (DDR2_BASEADDR + 0x78)
#define DLL_STATUS_REG                         (DDR2_BASEADDR + 0x7c)
#define DQS_GATE_PHASE_REG                     (DDR2_BASEADDR + 0x80)
#define DDR_PHY_RD_CTRL0_REG                   (DDR2_BASEADDR + 0x84)
#define DDR_PHY_RD_CTRL1_REG                   (DDR2_BASEADDR + 0x88)
#define STREAM_REG                             (DDR2_BASEADDR + 0x8c)
#define CUST_PT0_REG                           (DDR2_BASEADDR + 0x90)
#define CUST_PT1_REG                           (DDR2_BASEADDR + 0x94)
#define CUST_PT2_REG                           (DDR2_BASEADDR + 0x98)
#define CUST_PT3_REG                           (DDR2_BASEADDR + 0x9c)
#define CMPFIFO_STS_REG                        (DDR2_BASEADDR + 0xa0)
#define LAST_RD_DATA0_REG                      (DDR2_BASEADDR + 0xa4)
#define LAST_RD_DATA1_REG                      (DDR2_BASEADDR + 0xa8)
#define LAST_RD_DATA2_REG                      (DDR2_BASEADDR + 0xac)
#define LAST_RD_DATA3_REG                      (DDR2_BASEADDR + 0xb0)
#define LAST_RD_DATA4_REG                      (DDR2_BASEADDR + 0xb4)
#define LAST_RD_DATA5_REG                      (DDR2_BASEADDR + 0xb8)
#define LAST_RD_DATA6_REG                      (DDR2_BASEADDR + 0xbc)
#define LAST_RD_DATA7_REG                      (DDR2_BASEADDR + 0xc0)
#define PRBS_TEST_REG                          (DDR2_BASEADDR + 0xc4)
#define ISERDES_RFALL0_REG                     (DDR2_BASEADDR + 0xc8)
#define ISERDES_WFALL0_REG                     (DDR2_BASEADDR + 0xcc)
#define ISERDES_RRISE0_REG                     (DDR2_BASEADDR + 0xd0)
#define ISERDES_WRISE0_REG                     (DDR2_BASEADDR + 0xd4)
#define ISERDES_RFALL1_REG                     (DDR2_BASEADDR + 0xd8)
#define ISERDES_WFALL1_REG                     (DDR2_BASEADDR + 0xdc)
#define ISERDES_RRISE1_REG                     (DDR2_BASEADDR + 0xe0)
#define ISERDES_WRISE1_REG                     (DDR2_BASEADDR + 0xe4)

#define DDR_REG_WIDTH                          4

union athr_ddr_ctrl_misc_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:12;
        uint32_t TIMER_DONE:1;
        uint32_t HOST_IDLE_MODE:1;
        uint32_t CKE:1;
        uint32_t REOD_BP:1;
        uint32_t CTRL_QUEUE_EMPTY:1;
        uint32_t HOST_PORT_DISABLE:1;
        uint32_t DATA_TRAIN_STATE:1;
        uint32_t INIT_STATE:1;
        uint32_t SRFSH_STATE:1;
        uint32_t POWER_DOWN_STATE:1;
        uint32_t ADDR_MAP_MODE:2;
        uint32_t RESERVED1:1;
        uint32_t DENSITY:3;
        uint32_t DDR_MODE:2;
        uint32_t ALN:2;
    }content;
};


union athr_mmu_l0_arb_reg{
    uint32_t val;
    struct {
        uint32_t H3_MAXCREDIT:8;
        uint32_t H2_MAXCREDIT:8;
        uint32_t H1_MAXCREDIT:8;
        uint32_t H0_MAXCREDIT:8;
    }content;
};

union athr_mmu_l1_arb0_reg{
    uint32_t val;
    struct {
        uint32_t P3_MAXCREDIT:8;
        uint32_t P2_MAXCREDIT:8;
        uint32_t P1_MAXCREDIT:8;
        uint32_t P0_MAXCREDIT:8;
    }content;
};

union athr_mmu_l1_arb1_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:17;
        uint32_t L1_ARB_MODE:1;
        uint32_t PQUEUE_INTR_EN:4;
        uint32_t TIMEOUTM:2;
        uint32_t TIMEOUT:8;
    }cotent;
};

union athr_mmu_l2_arb0_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:31;
        uint32_t SUPERPORT_EN:1;
    }content;
};

union athr_mmu_pri_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:23;
        uint32_t MMU_CFG_LOAD:1;
        uint32_t HOST_PRI:8;
    }content;
};

union athr_ddr_config0_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:1;
        uint32_t CL:4;
        uint32_t TRRD:4;
        uint32_t RESERVED1:4;
        uint32_t TRP:4;
        uint32_t TRC:6;
        uint32_t TRCD:4;
        uint32_t TRAS:5;
    }content;
};

union athr_ddr_config1_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:11;
        uint32_t TWTR:3;
        uint32_t TRTW:1;
        uint32_t TWR:3;
        uint32_t TRTP:3;
        uint32_t TXP:5;
        uint32_t TCKE:1;
        uint32_t BURST_TYPE:1;
        uint32_t BURST_LENGTH:4;
    }content;
};

union athr_ddr_config2_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:7;
        uint32_t TCCD:1;
        uint32_t TRFC:8;
        uint32_t TMOD:4;
        uint32_t TMRD:2;
        uint32_t TXS:10;
    }content;
};

union athr_ddr_mode_register_reg{
    uint32_t val;
    struct {
        uint32_t MRS:1;
        uint32_t RESERVED0:18;
        uint32_t RESERVED1:1;
        uint32_t WR:3;
        uint32_t DLL:1;
        uint32_t TM:1;
        uint32_t CL:3;
        uint32_t BT:1;
        uint32_t BURSTLENGTH:3;
    }content;
};

union athr_ddr3_mode_register_reg{
    uint32_t val;
    struct {
        uint32_t MRS:1;
        uint32_t RESERVED0:18;
        uint32_t PD:1;
        uint32_t WR:3;
        uint32_t DLL:1;
        uint32_t RESERVERD2:1;
        uint32_t CL1:3;
        uint32_t BT:1;
        uint32_t CL0:1;
        uint32_t BURSTLENGTH:2;
    }content;
};

union athr_ddr_extend_mode_register1_reg{
    uint32_t val;
    struct {
        uint32_t EMR1S:1;
        uint32_t RESERVED0:15;
        uint32_t EMR1_VALUE:16;
#if 0
        uint32_t OUT:1;
        uint32_t RDQS:1;
        uint32_t DQS:1;
        uint32_t OCD_PROGRAM:3;
        uint32_t RIT1:1;
        uint32_t POSTED_CAS:3;
        uint32_t RIT0:1;
        uint32_t ODS:1;
        uint32_t DLL:1;
#endif
    }content;
};
union athr_ddr_extend_mode_register2_reg{
    uint32_t val;
    struct {
        uint32_t EMR2S:1;
        uint32_t RESERVED0:18;
        uint32_t EMR2_VALUE:13;
#if 0
        uint32_t RESERVED1:5;
        uint32_t SRT:1;
        uint32_t RESERVED2:7;
#endif
    }content;
};

union athr_ddr_extend_mode_register3_reg{
    uint32_t val;
    struct {
        uint32_t EMR3S:1;
        uint32_t RESERVED0:18;
        uint32_t EMR3_VALUE:13;
#if 0
        uint32_t RESERVED1:13;
#endif
    }content;
};

union athr_ddr_refresh_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:11;
        uint32_t REF_BURST:4;
        uint32_t REF_EN:1;
        uint32_t REF_PERIOD:16;
    }content;
};

union athr_ddr2_config_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:15;
        uint32_t DDR2_AL:4;
        uint32_t RESERVED1:3;
        uint32_t DDR2_ODT:1;
        uint32_t RESERVED2:1;
        uint32_t TFAW:6;
        uint32_t RESERVED3:2;
    }content;
};

union athr_ddr_cfg_reg{
    uint32_t val;
    struct {
        uint32_t CMD_ISSUE:1;
        uint32_t DT_DONE:1;
        uint32_t INIT_DONE:1;
        uint32_t RESERVED0:1;
        uint32_t RD_DATA_VLD:1;
        uint32_t SP_CMD_EXIT:1;
        uint32_t SP_CMD_ENTRY:1;
        uint32_t DT_START:1;
        uint32_t INIT_START:1;
        uint32_t CMD:4;
        uint32_t ADDR:16;
        uint32_t BA:3;
    }content;
};

union athr_ddr_dt_addr_reg{
    uint32_t val;
    struct {
        uint32_t DT_ADDR:32;
    }content;
};

union athr_ddr_dt0_reg{
    uint32_t val;
    struct {
        uint32_t DT_DATA0:32;
    }content;
};

union athr_ddr_dt1_reg{
    uint32_t val;
    struct {
        uint32_t DT_DATA1:32;
    }content;
};

union athr_ddr_dt2_reg{
    uint32_t val;
    struct {
        uint32_t DT_DATA2:32;
    }content;
};

union athr_ddr_dt3_reg{
    uint32_t val;
    struct {
        uint32_t DT_DATA3:32;
    }content;
};

union athr_ddr_dt4_reg{
    uint32_t val;
    struct {
        uint32_t DT_DATA4:32;
    }content;
};

union athr_ddr_dt5_reg{
    uint32_t val;
    struct {
        uint32_t DT_DATA5:32;
    }content;
};

union athr_ddr_dt6_reg{
    uint32_t val;
    struct {
        uint32_t DT_DATA6:32;
    }content;
};

union athr_ddr_dt7_reg{
    uint32_t val;
    struct {
        uint32_t DT_DATA7:32;
    }content;
};

union athr_ddr_timer_reg{
    uint32_t val;
    struct {
        uint32_t COUNTER:32;
    }content;
};

union athr_ddr_min_reg{
    uint32_t val;
    struct {
        uint32_t ADDR:32;
    }content;
};

union athr_ddr_max_reg {
    uint32_t val;
    struct {
        uint32_t ADDR:32;
    }content;
};

union athr_ddr_exception_reg {
    uint32_t val;
    struct {
        uint32_t RESERVED:25;
        uint32_t EX_RDATA_EXCP:1;
        uint32_t EX_WDATA_EXCP:1;
        uint32_t EX_TAG_EXCP:1;
        uint32_t EX_CMD_EXCP:1;
        uint32_t RRESP_EXCP:1;
        uint32_t WRESP_EXCP:1;
        uint32_t INTR:1;
    }content;
};

union athr_ddr_phy_misc_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:15;
        uint32_t ISERDES_FLUSH:1;
        uint32_t PHY_SW_RST:1;
        uint32_t RESERVED1:1;
        uint32_t CNTL_OE_EN:1;
        uint32_t IDDQ_EN:1;
        uint32_t DRAM_CLK_DIS:1;
        uint32_t INT_STI_EN:1;
        uint32_t RESERVED2:2;
        uint32_t EXT_LB_EN:1;
        uint32_t ANALOG_RST:1;
        uint32_t DDR_MODE:2;
        uint32_t CODE_SET_MANU_EN:1;
        uint32_t RESERVED3:3;
    }content;
};

union athr_dll_ctrl_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:26;
        uint32_t LEFT_BW_SEL:2;
        uint32_t RIGHT_BW_SEL:2;
        uint32_t UP_BW_SEL:2;
    }content;
};

union athr_dll_status_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:23;
        uint32_t DLL_LOCKED:1;
        uint32_t RESERVED1:1;
        uint32_t FIRST_LOCK_FLAG:1;
        uint32_t DLL_CODE:6;
    }content;
};



union athr_dqs_gate_phase_reg{
    int32_t val;
    struct {
        uint32_t RESERVED:16;
        uint32_t DQS_IN_GATE_PHASE_LANE3:2;
        uint32_t DQS_IN_GATE_PHASE_LANE2:2;
        uint32_t DQS_IN_GATE_PHASE_LANE1:2;
        uint32_t DQS_IN_GATE_PHASE_LANE0:2;
        uint32_t DQS_OUT_GATE_PHASE_LANE3:2;
        uint32_t DQS_OUT_GATE_PHASE_LANE2:2;
        uint32_t DQS_OUT_GATE_PHASE_LANE1:2;
        uint32_t DQS_OUT_GATE_PHASE_LANE0:2;
    }content;
};

union athr_ddr_phy_rd_ctrl0_reg{
    int32_t val;
    struct {
        uint32_t RESERVED1:17;
        uint32_t DQS_IN_EDLY_CODE_OFF1:7;
        uint32_t RESERVED0:1;
        uint32_t DQS_IN_EDLY_CODE_OFF0:7;
    }content;
};

union athr_ddr_phy_rd_ctrl1_reg{
    int32_t val;
    struct {
        uint32_t RESERVED3:1;
        uint32_t DQS_IN_FDLY_CODE_OFF1:7;
        uint32_t RESERVED2:1;
        uint32_t DQS_IN_FDLY_CODE_OFF0:7;
        uint32_t RESERVED1:1;
        uint32_t DQS_IN_RDLY_CODE_OFF1:7;
        uint32_t RESERVED0:1;
        uint32_t DQS_IN_RDLY_CODE_OFF0:7;
    }content;
};

union athr_stream_ctrl_reg {
    uint32_t val;
    struct {
        int32_t RESERVED:30;
        int32_t CMP_RESULT:1;
        int32_t BUSY:1;
    }content;
};

union athr_cust_pt0_reg{
    uint32_t val;
    struct {
        uint32_t PT0:32;
    }content;
};

union athr_cust_pt1_reg{
    uint32_t val;
    struct {
        uint32_t PT1:32;
    }content;
};

union athr_cust_pt2_reg{
    uint32_t val;
    struct {
        uint32_t PT2:32;
    }content;
};

union athr_cust_pt3_reg{
    uint32_t val;
    struct {
        uint32_t PT3:32;
    }content;
};

union athr_cmpfifo_sts_reg{
    uint32_t val;
    struct {
        uint32_t RESERVED0:21;
        uint32_t EMPTY:1;
        uint32_t FULL:1;
        uint32_t HAPPENED:1;
        uint32_t TRACK_CNT:8;
    }content;
};

union athr_last_rd_data0_reg{
    uint32_t val;
    struct {
        uint32_t DATA0:32;
    }content;
};

union athr_last_rd_data1_reg{
    uint32_t val;
    struct {
        uint32_t DATA1:32;
    }content;
};

union athr_last_rd_data2_reg{
    uint32_t val;
    struct {
        uint32_t DATA2:32;
    }content;
};

union athr_last_rd_data3_reg{
    uint32_t val;
    struct {
        uint32_t DATA3:32;
    }content;
};
union athr_last_rd_data4_reg{
    uint32_t val;
    struct {
        uint32_t DATA4:32;
    }content;
};

union athr_last_rd_data5_reg{
    uint32_t val;
    struct {
        uint32_t DATA5:32;
    }content;
};

union athr_last_rd_data6_reg{
    uint32_t val;
    struct {
        uint32_t DATA6:32;
    }content;
};

union athr_last_rd_data7_reg{
    uint32_t val;
    struct {
        uint32_t DATA7:32;
    }content;
};

union athr_prbs_test_reg {
    uint32_t val;
    struct {
        uint32_t PRBS_ERR_CNT:16;
        uint32_t RESERVED:14;
        uint32_t PRBS_FAIL:1;
        uint32_t PRBS_EN:1;
    }content;
};

union athr_iserdes_rfall0_reg {
    uint32_t val;
    struct {
        uint32_t RINF:32;
    }content;
};

union athr_iserdes_wfall0_reg {
    uint32_t val;
    struct {
        uint32_t WINF:32;
    }content;
};

union athr_iserdes_rrise0_reg {
    uint32_t val;
    struct {
        uint32_t RINF:32;
    }content;
};

union athr_iserdes_wrise0_reg {
    uint32_t val;
    struct {
        uint32_t WINF:32;
    }content;
};

union athr_iserdes_rfall1_reg {
    uint32_t val;
    struct {
        uint32_t RINF:32;
    }content;
};

union athr_iserdes_wfall1_reg {
    uint32_t val;
    struct {
        uint32_t WINF:32;
    }content;
};

union athr_iserdes_rrise1_reg {
    uint32_t val;
    struct {
        uint32_t RINF:32;
    }content;
};

union athr_iserdes_wrise1_reg {
    uint32_t val;
    struct {
        uint32_t WINF:32;
    }content;
};



int32_t athr_ddr_config_dump(struct athr_ddr_init_config dump_config);
int athr_ddr_init_datatrain(struct athr_ddr_init_config athr_ddr_init_val);
void athr_ddr1_init(void);
void athr_ddr2_init(void);
void athr_ddr3_init(void);
void athr_sdram_init(void);

int32_t set_ddr_speed(uint32_t ddr_speed);
int32_t get_ddr_speed(uint32_t * ddr_speed);
int32_t ddr_timer_delay(uint32_t delay);
#endif
