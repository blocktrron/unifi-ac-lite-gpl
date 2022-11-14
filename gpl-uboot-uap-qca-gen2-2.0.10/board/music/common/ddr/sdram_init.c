#include "ddr.h"

static int32_t
athr_sdram_config_init(struct athr_ddr_init_config athr_ddr_init_val)
{
    int32_t rtn_val = 0;

    union athr_ddr_timer_reg                 athr_ddr_timer_init;
    union athr_ddr_ctrl_misc_reg             athr_ddr_ctrl_misc_init;
    union athr_ddr_cfg_reg                   athr_ddr_cfg_init;
    union athr_ddr_extend_mode_register3_reg athr_ddr_extend_mode_register3_init;
    union athr_ddr_extend_mode_register2_reg athr_ddr_extend_mode_register2_init;
    union athr_ddr_extend_mode_register1_reg athr_ddr_extend_mode_register1_init;
    union athr_ddr_mode_register_reg         athr_ddr_mode_register_init;
    union athr_ddr_config2_reg               athr_ddr_config2_init;
    union athr_ddr_config1_reg               athr_ddr_config1_init;
    union athr_ddr_config0_reg               athr_ddr_config0_init;
    union athr_ddr_refresh_reg               athr_ddr_refresh_init;
    union athr_ddr_phy_rd_ctrl0_reg          phy_rd_ctrl0_init;
    union athr_ddr_phy_rd_ctrl1_reg          phy_rd_ctrl1_init;
    union athr_ddr_phy_misc_reg              phy_misc_init;
    union athr_dqs_gate_phase_reg            dqs_gate_phase_init;

    memset(&athr_ddr_timer_init,                     0, sizeof(athr_ddr_timer_init));
    memset(&athr_ddr_ctrl_misc_init,                 0, sizeof(athr_ddr_ctrl_misc_init));
    memset(&athr_ddr_cfg_init,                       0, sizeof(athr_ddr_cfg_init));
    memset(&athr_ddr_extend_mode_register3_init,     0, sizeof(athr_ddr_extend_mode_register3_init));
    memset(&athr_ddr_extend_mode_register2_init,     0, sizeof(athr_ddr_extend_mode_register2_init));
    memset(&athr_ddr_extend_mode_register1_init,     0, sizeof(athr_ddr_extend_mode_register1_init));
    memset(&athr_ddr_mode_register_init,             0, sizeof(athr_ddr_mode_register_init));
    memset(&athr_ddr_config2_init,                   0, sizeof(athr_ddr_config2_init));
    memset(&athr_ddr_config1_init,                   0, sizeof(athr_ddr_config1_init));
    memset(&athr_ddr_config0_init,                   0, sizeof(athr_ddr_config0_init));
    memset(&athr_ddr_refresh_init,                   0, sizeof(athr_ddr_refresh_init));
    memset(&phy_misc_init,                           0, sizeof(phy_misc_init));
    memset(&dqs_gate_phase_init,                     0, sizeof(dqs_gate_phase_init));
    memset(&phy_rd_ctrl0_init,                       0, sizeof(phy_rd_ctrl0_init));
    memset(&phy_rd_ctrl1_init,                       0, sizeof(phy_rd_ctrl1_init));

    /* 1 waits 200us cycles for the PHY DLLS to lock */
    ddr_timer_delay(200);

    /* 2 Apply NOP and set CKE high */
    athr_ddr_ctrl_misc_init.val = music_reg_rd(DDR_CTRL_MISC_REG);
    athr_ddr_ctrl_misc_init.content.ALN      = athr_ddr_init_val.lane_number;
    athr_ddr_ctrl_misc_init.content.DDR_MODE = athr_ddr_init_val.ddr_mode;
    athr_ddr_ctrl_misc_init.content.DENSITY  = athr_ddr_init_val.density;
    athr_ddr_ctrl_misc_init.content.ADDR_MAP_MODE = athr_ddr_init_val.addr_map_mode;
    athr_ddr_ctrl_misc_init.content.CKE = 1;
    athr_ddr_ctrl_misc_init.content.INIT_STATE = 1;
    music_reg_wr_nf(DDR_CTRL_MISC_REG, athr_ddr_ctrl_misc_init.val);

    DDR_DEBUG("3:addr = 0x%x, val = 0x%x\r\n", DDR_CTRL_MISC_REG,
        athr_ddr_ctrl_misc_init.val);

    /* 3 wait 400 us cycles */
    ddr_timer_delay(400);

     /* DDR MR register WR value must be the same with the tWR value in the DDR_Config1
       DDR MR register burst_type & burst_length should set the same with DDR_Config1 register
       CL bit value should set the same in DDR_config0 register and DDR_MR register
     */

    athr_ddr_config2_init.val = music_reg_rd(DDR_CONFIG2_REG);
    athr_ddr_config2_init.content.TRFC = athr_ddr_init_val.trfc;
    music_reg_wr_nf(DDR_CONFIG2_REG, athr_ddr_config2_init.val);
    DDR_DEBUG("3:addr = 0x%x, val = 0x%x\r\n", DDR_CONFIG2_REG,
        athr_ddr_config2_init.val);

    athr_ddr_config1_init.val = music_reg_rd(DDR_CONFIG1_REG);
    athr_ddr_config1_init.content.TWR = athr_ddr_init_val.write_recovery;
    athr_ddr_config1_init.content.BURST_TYPE = athr_ddr_init_val.burst_type;
    if(athr_ddr_init_val.burst_length == DDR2_BURSTLENGTH_4) {
        athr_ddr_config1_init.content.BURST_LENGTH = 4;
    } else if (athr_ddr_init_val.burst_length == DDR2_BURSTLENGTH_8){
        athr_ddr_config1_init.content.BURST_LENGTH = 8;
    }
    music_reg_wr_nf(DDR_CONFIG1_REG, athr_ddr_config1_init.val);

    DDR_DEBUG("3:addr = 0x%x, val = 0x%x\r\n", DDR_CONFIG1_REG,
        athr_ddr_config1_init.val);

    athr_ddr_config0_init.val = music_reg_rd(DDR_CONFIG0_REG);
    athr_ddr_config0_init.content.CL = athr_ddr_init_val.CAS_latency;
    athr_ddr_config0_init.content.TRRD = athr_ddr_init_val.trrd;
    athr_ddr_config0_init.content.TRP = athr_ddr_init_val.trp;
    athr_ddr_config0_init.content.TRC = athr_ddr_init_val.trc;
    athr_ddr_config0_init.content.TRCD = athr_ddr_init_val.trcd;
    athr_ddr_config0_init.content.TRAS = athr_ddr_init_val.tras;
    music_reg_wr_nf(DDR_CONFIG0_REG, athr_ddr_config0_init.val);

    DDR_DEBUG("3:addr = 0x%x, val = 0x%x\r\n", DDR_CONFIG0_REG,
        athr_ddr_config0_init.val);

    athr_ddr_refresh_init.val = music_reg_rd(DDR_REFRESH_REG);
    athr_ddr_refresh_init.content.REF_BURST = 1;
    athr_ddr_refresh_init.content.REF_EN = 1;
    athr_ddr_refresh_init.content.REF_PERIOD = (600*
            athr_ddr_init_val.clock * 19) / 1600;
    //athr_ddr_refresh_init.content.REF_PERIOD= 0x8000;
    music_reg_wr_nf(DDR_REFRESH_REG, athr_ddr_refresh_init.val);
    DDR_DEBUG("3:addr = 0x%x, val = 0x%x\r\n", DDR_REFRESH_REG,
        athr_ddr_refresh_init.val);


    phy_misc_init.content.CODE_SET_MANU_EN = 1;
    phy_misc_init.content.CNTL_OE_EN = 1;
    music_reg_wr_nf(DDR_PHY_MISC_REG, phy_misc_init.val);
    DDR_DEBUG("3:addr = 0x%x, val = 0x%x \n", DDR_PHY_MISC_REG,
        phy_misc_init.val);
#if 1
    dqs_gate_phase_init.content.DQS_IN_GATE_PHASE_LANE3 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_IN_GATE_PHASE_LANE2 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_IN_GATE_PHASE_LANE1 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_IN_GATE_PHASE_LANE0 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_OUT_GATE_PHASE_LANE3 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_OUT_GATE_PHASE_LANE2 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_OUT_GATE_PHASE_LANE1 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_OUT_GATE_PHASE_LANE0 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
#else
    dqs_gate_phase_init.val = 0;
    music_reg_wr_nf(DQS_GATE_PHASE_REG, dqs_gate_phase_init.val);
#endif
    DDR_DEBUG("3: addr = 0x%x, val = 0x%x \n", DQS_GATE_PHASE_REG,
        dqs_gate_phase_init.val);
#if 1
    phy_rd_ctrl0_init.content.DQS_IN_EDLY_CODE_OFF1 = 0x3f;
    phy_rd_ctrl0_init.content.DQS_IN_EDLY_CODE_OFF0 = 0x3f;
#else
    phy_rd_ctrl0_init.val = 0;
    music_reg_wr_nf(DDR_PHY_RD_CTRL0_REG, phy_rd_ctrl0_init.val);
#endif
    DDR_DEBUG("3: addr = 0x%x, val = 0x%x \n", DDR_PHY_RD_CTRL0_REG,
        phy_rd_ctrl0_init.val);
#if 0
    phy_rd_ctrl1_init.content.DQS_IN_RDLY_CODE_OFF1 =
        DEFAULT_DQS_IN_RDLY_CODE_OFF;
    phy_rd_ctrl1_init.content.DQS_IN_RDLY_CODE_OFF0 =
        DEFAULT_DQS_IN_RDLY_CODE_OFF;
    phy_rd_ctrl1_init.content.DQS_IN_FDLY_CODE_OFF1 =
        DEFAULT_DQS_IN_FDLY_CODE_OFF;
    phy_rd_ctrl1_init.content.DQS_IN_FDLY_CODE_OFF0 =
        DEFAULT_DQS_IN_FDLY_CODE_OFF;
#else
    phy_rd_ctrl1_init.val = 0x20202020;
    music_reg_wr_nf(DDR_PHY_RD_CTRL1_REG, phy_rd_ctrl1_init.val);
#endif
    DDR_DEBUG("3: addr = 0x%x, val = 0x%x \n", DDR_PHY_RD_CTRL1_REG,
        phy_rd_ctrl1_init.val);

    /* 4 Issue a percharge all command */
    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.CMD = CMD_PRECHARGE_ALL;
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;

    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);
    DDR_DEBUG("4:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    /* 5  Issue two REFRESH commands */
    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.CMD = CMD_REFRESH;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("5:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.CMD = CMD_REFRESH;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("5:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 6 Issue a load Mode Register command to reset DLL */
    athr_ddr_mode_register_init.val = music_reg_rd(DDR_MODE_REGISTER_REG);
    athr_ddr_mode_register_init.content.MRS = 0;
    athr_ddr_mode_register_init.content.CL = athr_ddr_init_val.CAS_latency;
    athr_ddr_mode_register_init.content.DLL = 0;
    athr_ddr_mode_register_init.content.BURSTLENGTH = athr_ddr_init_val.burst_length;
    music_reg_wr_nf(DDR_MODE_REGISTER_REG, athr_ddr_mode_register_init.val);

    DDR_DEBUG("6:addr = 0x%x, val = 0x%x\r\n", DDR_MODE_REGISTER_REG,
        athr_ddr_mode_register_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.BA = 0;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("6:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 7 set init done reg */
    memset(&athr_ddr_cfg_init.val, 0, sizeof(athr_ddr_cfg_init));

    athr_ddr_cfg_init.content.INIT_DONE = 1;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("7:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    return  rtn_val;
}

static struct athr_ddr_init_config default_sdram_config = {
    .CAS_latency    = 3,
    .write_recovery = 2,
    .burst_length   = DDR2_BURSTLENGTH_4,
    .burst_type     = DDR_BURSTTYPE_SEQUENTIAL,
    .ddr_mode       = SDR_DRAM_MODE,
    .lane_number    = DDR_ALN_ONE_LANE,
    //.density        = DDR_128MB,
	.density        = DDR_256MB,
    .trfc           = 8,
    .trrd           = 2,
    .trp            = 3,
    .trc            = 7,
    .trcd           = 3,
    .tras           = 10,
    .clock          = 100,
};

void ddr_init(void)
{
    set_ddr_speed(default_sdram_config.clock);
    athr_sdram_config_init(default_sdram_config);
    //athr_ddr_init_datatrain(default_sdram_config);
}
