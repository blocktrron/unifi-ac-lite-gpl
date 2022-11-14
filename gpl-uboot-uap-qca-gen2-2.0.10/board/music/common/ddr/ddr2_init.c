#include "ddr.h"

static int32_t
athr_ddr2_config_init(struct athr_ddr_init_config athr_ddr_init_val)
{
    int32_t rtn_val = 0;
    uint32_t odt_reg_val = 0;
    union athr_ddr_timer_reg                 athr_ddr_timer_init;
    union athr_ddr_ctrl_misc_reg             athr_ddr_ctrl_misc_init;
    union athr_ddr_cfg_reg                   athr_ddr_cfg_init;
    union athr_ddr_extend_mode_register3_reg athr_ddr_extend_mode_register3_init;
    union athr_ddr_extend_mode_register2_reg athr_ddr_extend_mode_register2_init;
    union athr_ddr_extend_mode_register1_reg athr_ddr_extend_mode_register1_init;
    union athr_ddr_mode_register_reg         athr_ddr_mode_register_init;
    union athr_ddr_config1_reg               athr_ddr_config1_init;
    union athr_ddr_config0_reg               athr_ddr_config0_init;
    union athr_ddr_config2_reg               athr_ddr_config2_init;
    union athr_ddr_refresh_reg               athr_ddr_refresh_init;
    union athr_ddr_phy_rd_ctrl0_reg         phy_rd_ctrl0_init;
    union athr_ddr_phy_rd_ctrl1_reg         phy_rd_ctrl1_init;
    union athr_ddr_phy_misc_reg             phy_misc_init;
    union athr_dqs_gate_phase_reg           dqs_gate_phase_init;
	union athr_ddr2_config_reg 				ddr2_config_init;

    memset(&athr_ddr_timer_init,                     0, sizeof(athr_ddr_timer_init));
    memset(&athr_ddr_ctrl_misc_init,                 0, sizeof(athr_ddr_ctrl_misc_init));
    memset(&athr_ddr_cfg_init,                       0, sizeof(athr_ddr_cfg_init));
    memset(&athr_ddr_extend_mode_register3_init,     0, sizeof(athr_ddr_extend_mode_register3_init));
    memset(&athr_ddr_extend_mode_register2_init,     0, sizeof(athr_ddr_extend_mode_register2_init));
    memset(&athr_ddr_extend_mode_register1_init,     0, sizeof(athr_ddr_extend_mode_register1_init));
    memset(&athr_ddr_mode_register_init,             0, sizeof(athr_ddr_mode_register_init));
    memset(&athr_ddr_config1_init,                   0, sizeof(athr_ddr_config1_init));
    memset(&athr_ddr_config0_init,                   0, sizeof(athr_ddr_config0_init));
    memset(&athr_ddr_refresh_init,                   0, sizeof(athr_ddr_refresh_init));
    memset(&athr_ddr_config2_init,                   0, sizeof(athr_ddr_config2_init));
    memset(&dqs_gate_phase_init,                     0, sizeof(dqs_gate_phase_init));
    memset(&phy_misc_init,                           0, sizeof(phy_misc_init));
    memset(&phy_rd_ctrl0_init,                       0, sizeof(phy_rd_ctrl0_init));
    memset(&phy_rd_ctrl1_init,                       0, sizeof(phy_rd_ctrl1_init));
    memset(&ddr2_config_init,                        0, sizeof(ddr2_config_init));

    if(athr_ddr_init_val.odt == 0) {
        odt_reg_val = 0;
    } else if(athr_ddr_init_val.odt == 50) {
        odt_reg_val = 0x44;
    } else if(athr_ddr_init_val.odt == 75) {
        odt_reg_val = 0x40;
    } else if(athr_ddr_init_val.odt == 150) {
        odt_reg_val = 0x04;
    }
    /* 1 waits 200us cycles for the PHY DLLS to lock */
    ddr_timer_delay(200);

    /* 3 Apply NOP and set CKE high */
    athr_ddr_ctrl_misc_init.val = music_reg_rd(DDR_CTRL_MISC_REG);
    athr_ddr_ctrl_misc_init.content.ALN      = athr_ddr_init_val.lane_number;
    athr_ddr_ctrl_misc_init.content.DDR_MODE = athr_ddr_init_val.ddr_mode;
    athr_ddr_ctrl_misc_init.content.DENSITY  = athr_ddr_init_val.density;
    athr_ddr_ctrl_misc_init.content.ADDR_MAP_MODE = athr_ddr_init_val.addr_map_mode;
    athr_ddr_ctrl_misc_init.content.CKE = 1;
    athr_ddr_ctrl_misc_init.content.INIT_STATE = 1;
    music_reg_wr_nf(DDR_CTRL_MISC_REG, athr_ddr_ctrl_misc_init.val);

    DDR_DEBUG("3:addr = 0x%x, val = 0x%x\r\n", DDR_CTRL_MISC_REG, athr_ddr_ctrl_misc_init.val);

    athr_ddr_config0_init.val = music_reg_rd(DDR_CONFIG0_REG);
    athr_ddr_config0_init.content.CL = athr_ddr_init_val.CAS_latency;
    athr_ddr_config0_init.content.TRRD = athr_ddr_init_val.trrd;
    athr_ddr_config0_init.content.TRP = athr_ddr_init_val.trp;
    athr_ddr_config0_init.content.TRC  = athr_ddr_init_val.trc;
    athr_ddr_config0_init.content.TRCD = athr_ddr_init_val.trcd;
    athr_ddr_config0_init.content.TRAS = athr_ddr_init_val.tras;
    music_reg_wr_nf(DDR_CONFIG0_REG, athr_ddr_config0_init.val);

    DDR_DEBUG("3:addr = 0x%x, val = 0x%x\r\n", DDR_CONFIG0_REG,
        athr_ddr_config0_init.val);

    athr_ddr_config1_init.val = music_reg_rd(DDR_CONFIG1_REG);
    athr_ddr_config1_init.content.TWTR = athr_ddr_init_val.twtr;
    athr_ddr_config1_init.content.TRTW = athr_ddr_init_val.trtw;
    athr_ddr_config1_init.content.TWR = athr_ddr_init_val.write_recovery;
    athr_ddr_config1_init.content.TRTP = athr_ddr_init_val.trtp;
    athr_ddr_config1_init.content.BURST_TYPE = athr_ddr_init_val.burst_type;
    if(athr_ddr_init_val.burst_length == DDR2_BURSTLENGTH_4) {
        athr_ddr_config1_init.content.BURST_LENGTH = 4;
    } else if (athr_ddr_init_val.burst_length == DDR2_BURSTLENGTH_8){
        athr_ddr_config1_init.content.BURST_LENGTH = 8;
    }
    music_reg_wr_nf(DDR_CONFIG1_REG, athr_ddr_config1_init.val);

    DDR_DEBUG("3:addr = 0x%x, val = 0x%x\r\n", DDR_CONFIG1_REG,
        athr_ddr_config1_init.val);

    athr_ddr_config2_init.val = music_reg_rd(DDR_CONFIG2_REG);
    athr_ddr_config2_init.content.TCCD = athr_ddr_init_val.tccd;
    athr_ddr_config2_init.content.TRFC = athr_ddr_init_val.trfc;
    athr_ddr_config2_init.content.TMOD = athr_ddr_init_val.tmod;
    music_reg_wr(DDR_CONFIG2_REG, athr_ddr_config2_init.val);
    DDR_DEBUG("3: addr = 0x%x, val = 0x%x \r\n", DDR_CONFIG2_REG, athr_ddr_config2_init.val);

    athr_ddr_refresh_init.val = music_reg_rd(DDR_REFRESH_REG);
    athr_ddr_refresh_init.content.REF_BURST = 7;
    athr_ddr_refresh_init.content.REF_EN = 1;
    athr_ddr_refresh_init.content.REF_PERIOD = (620 * athr_ddr_init_val.clock * 19) / 200;

    music_reg_wr_nf(DDR_REFRESH_REG, athr_ddr_refresh_init.val);

    DDR_DEBUG("3: addr = 0x%x, val = 0x%x \r\n", DDR_REFRESH_REG, athr_ddr_refresh_init.val);

    phy_misc_init.content.CODE_SET_MANU_EN = 1;
    phy_misc_init.content.CNTL_OE_EN = 1;
    music_reg_wr_nf(DDR_PHY_MISC_REG, phy_misc_init.val);
    DDR_DEBUG("3: addr = 0x%x val = 0x%x \n", DDR_PHY_MISC_REG,
        phy_misc_init.val);

    dqs_gate_phase_init.content.DQS_IN_GATE_PHASE_LANE3 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_IN_GATE_PHASE_LANE2 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_IN_GATE_PHASE_LANE1 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_IN_GATE_PHASE_LANE0 =
        DEFAULT_DQS_IN_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_OUT_GATE_PHASE_LANE3 =
        DEFAULT_DQS_OUT_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_OUT_GATE_PHASE_LANE2 =
        DEFAULT_DQS_OUT_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_OUT_GATE_PHASE_LANE1 =
        DEFAULT_DQS_OUT_GATE_PHASE_LANE_NUM;
    dqs_gate_phase_init.content.DQS_OUT_GATE_PHASE_LANE0 =
        DEFAULT_DQS_OUT_GATE_PHASE_LANE_NUM;
    music_reg_wr_nf(DQS_GATE_PHASE_REG, dqs_gate_phase_init.val);
    DDR_DEBUG("3: addr = 0x%x val = 0x%x\n", DQS_GATE_PHASE_REG,
        dqs_gate_phase_init.val);

    phy_rd_ctrl0_init.content.DQS_IN_EDLY_CODE_OFF1 =
        DEFAULT_DQS_IN_EDLY_CODE_OFF;
    phy_rd_ctrl0_init.content.DQS_IN_EDLY_CODE_OFF0 =
        DEFAULT_DQS_IN_EDLY_CODE_OFF;
    music_reg_wr_nf(DDR_PHY_RD_CTRL0_REG, phy_rd_ctrl0_init.val);
    DDR_DEBUG("3: addr = 0x%x val = 0x%x\n", DDR_PHY_RD_CTRL0_REG,
        phy_rd_ctrl0_init.val);

    phy_rd_ctrl1_init.content.DQS_IN_RDLY_CODE_OFF1 =
        DEFAULT_DQS_IN_RDLY_CODE_OFF;
    phy_rd_ctrl1_init.content.DQS_IN_RDLY_CODE_OFF0 =
        DEFAULT_DQS_IN_RDLY_CODE_OFF;
    phy_rd_ctrl1_init.content.DQS_IN_FDLY_CODE_OFF1 =
        DEFAULT_DQS_IN_FDLY_CODE_OFF;
    phy_rd_ctrl1_init.content.DQS_IN_FDLY_CODE_OFF0 =
        DEFAULT_DQS_IN_FDLY_CODE_OFF;
    music_reg_wr_nf(DDR_PHY_RD_CTRL1_REG, phy_rd_ctrl1_init.val);
    DDR_DEBUG("3: addr = 0x%x val = 0x%x \n", DDR_PHY_RD_CTRL1_REG,
        phy_rd_ctrl1_init.val);

    /* 4 wait 400 us cycles */
    ddr_timer_delay(400);

    /* 5 Issue a percharge all command */
    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.CMD = CMD_PRECHARGE_ALL;
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;

    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("5:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG, athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 6 Issue a load Extended Mode Register 2 command */
    athr_ddr_extend_mode_register2_init.val = music_reg_rd(DDR_EXTENDED_MODE_REG2);
    athr_ddr_extend_mode_register2_init.content.EMR2S = 0;
    athr_ddr_extend_mode_register2_init.content.EMR2_VALUE = DDR2_EMR2VAL;
    music_reg_wr_nf(DDR_EXTENDED_MODE_REG2, athr_ddr_extend_mode_register2_init.val);

    DDR_DEBUG("6:addr = 0x%x, val = 0x%x\r\n", DDR_EXTENDED_MODE_REG2,
        athr_ddr_extend_mode_register2_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.BA = 2;
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("6:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 7 Issue a load Extended Mode Register 3 command */
    athr_ddr_extend_mode_register3_init.val = music_reg_rd(DDR_EXTENDED_MODE_REG3);
    athr_ddr_extend_mode_register3_init.content.EMR3S = 0;
    athr_ddr_extend_mode_register3_init.content.EMR3_VALUE = DDR2_EMR3VAL;
    music_reg_wr_nf(DDR_EXTENDED_MODE_REG3, athr_ddr_extend_mode_register3_init.val);

    DDR_DEBUG("7:addr = 0x%x, val = 0x%x\r\n", DDR_EXTENDED_MODE_REG3,
        athr_ddr_extend_mode_register3_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.BA = 3;
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("7:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 8 Issue a load Extended Mode Register 1 command to enable DLL */
    athr_ddr_extend_mode_register1_init.val = music_reg_rd(DDR_EXTENDED_MODE_REG1);
    athr_ddr_extend_mode_register1_init.content.EMR1S = 0;
    athr_ddr_extend_mode_register1_init.content.EMR1_VALUE = DDR2_EMR1VAL0;
    music_reg_wr_nf(DDR_EXTENDED_MODE_REG1, athr_ddr_extend_mode_register1_init.val);

    DDR_DEBUG("8:addr = 0x%x, val = 0x%x\r\n", DDR_EXTENDED_MODE_REG1,
        athr_ddr_extend_mode_register1_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.BA = 1;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);
    DDR_DEBUG("8:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);
    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 9 Issue a load Mode Register command to reset DLL */
    athr_ddr_mode_register_init.val = music_reg_rd(DDR_MODE_REG);
    athr_ddr_mode_register_init.content.MRS = 0;
    athr_ddr_mode_register_init.content.CL = athr_ddr_init_val.CAS_latency;
    athr_ddr_mode_register_init.content.DLL = 1;
    athr_ddr_mode_register_init.content.BURSTLENGTH = athr_ddr_init_val.burst_length;
    athr_ddr_mode_register_init.content.WR = athr_ddr_init_val.write_recovery;
    athr_ddr_mode_register_init.content.BT = athr_ddr_init_val.burst_type;
    music_reg_wr_nf(DDR_MODE_REG, athr_ddr_mode_register_init.val);

    DDR_DEBUG("9:addr = 0x%x, val = 0x%x\r\n", DDR_MODE_REG,
        athr_ddr_mode_register_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.BA = 0;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("9:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 10 Issue a percharge all command */
    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.CMD = CMD_PRECHARGE_ALL;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("10:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 11  Issue two REFRESH commands */
    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.CMD = CMD_REFRESH;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("11:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.CMD = CMD_REFRESH;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("11:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 12 Issue a load Mode Register command to program device
       operating parameters without reseting the DLL */
    athr_ddr_mode_register_init.val = music_reg_rd(DDR_MODE_REG);
    athr_ddr_mode_register_init.content.MRS = 0;
    athr_ddr_mode_register_init.content.DLL = 0;

    music_reg_wr_nf(DDR_MODE_REG, athr_ddr_mode_register_init.val);

    DDR_DEBUG("12:addr = 0x%x, val = 0x%x\r\n", DDR_MODE_REG,
        athr_ddr_mode_register_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.BA = 0;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("12:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);


    /* 13 wait 200 SDRAM clock cycles for the DLL to lock*/
    ddr_timer_delay(200);

    /* 14 Load EMR to program OCD to OCP operation default */
    athr_ddr_extend_mode_register1_init.val = music_reg_rd(DDR_EXTENDED_MODE_REG1);
    athr_ddr_extend_mode_register1_init.content.EMR1S = 0;
    athr_ddr_extend_mode_register1_init.content.EMR1_VALUE =
            DDR2_EMR1VAL1 | odt_reg_val;
    music_reg_wr_nf(DDR_EXTENDED_MODE_REG1, athr_ddr_extend_mode_register1_init.val);

    DDR_DEBUG("14:addr = 0x%x, val = 0x%x\r\n", DDR_EXTENDED_MODE_REG1,
        athr_ddr_extend_mode_register1_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.BA = 1;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("14:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 15 Issue a EMR load command to 4000 */
    athr_ddr_extend_mode_register1_init.val = music_reg_rd(DDR_EXTENDED_MODE_REG1);
    athr_ddr_extend_mode_register1_init.content.EMR1S = 0;
    athr_ddr_extend_mode_register1_init.content.EMR1_VALUE =
            DDR2_EMR1VAL2 | odt_reg_val;
    music_reg_wr_nf(DDR_EXTENDED_MODE_REG1, athr_ddr_extend_mode_register1_init.val);

    DDR_DEBUG("15:addr = 0x%x, val = 0x%x\r\n", DDR_EXTENDED_MODE_REG1,
        athr_ddr_extend_mode_register1_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.BA = 1;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("15:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 16 Set init done */
    memset(&athr_ddr_cfg_init.val, 0, sizeof(athr_ddr_cfg_init));

    athr_ddr_cfg_init.content.INIT_DONE = 1;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("16:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

	if(MUSIC_IS_VIVO_B0()) {
		ddr2_config_init.val = music_reg_rd(DDR_DDR2_CONFIG_REG);
		ddr2_config_init.content.DDR2_ODT = 1;
		music_reg_wr_nf(DDR_DDR2_CONFIG_REG, ddr2_config_init.val);
	}

    return  rtn_val;
}


#ifdef DEBUG_DDR_CONFIG
void ddr_init_cfg_input(struct athr_ddr_init_config *config)
{
    //serial_init();
    //serial_baudrate_set(115200);

    int  config_key = 0;
    printf("\nPlease select DDR clock:(0)100 (1)200 (2)266 (3)300 (4)400:");
    while (1) {
        config_key = 0;
		if (serial_tstc()) {
            config_key = serial_getc();
            if(config_key == 48) {
                config->clock = 100;
                break;
            } else if(config_key == 49) {
                config->clock = 200;
                break;
            } else if(config_key == 50) {
                config->clock = 266;
                break;
            } else if(config_key == 51) {
                config->clock = 300;
                break;
            } else if(config_key == 52) {
                config->clock = 400;
                break;
            } else {
                printf("Error DDR clock\n");
                printf("\nPlease select DDR clock:(0)100 (1)200 (2)266 (3)300 (4)400:");
            }
            //printf("\n");
        }
        udelay (10);
    }

    printf("\nPlease select DDR odt:(0)0 (1)50 (2)75 (3)150:");
    while (1) {
        config_key = 0;
		if (serial_tstc()) {
            config_key = serial_getc();
            if(config_key == 48) {
                config->odt = 0;
                break;
            } else if(config_key == 49) {
                config->odt = 50;
                break;
            } else if(config_key == 50) {
                config->odt = 75;
                break;
            } else if(config_key == 51) {
                config->odt = 150;
                break;
            } else {
                printf("Error DDR odt\n");
                printf("\nPlease select DDR odt:(0)0 (1)50 (2)75 (3)150:");
            }
           // printf("\n");
        }
        udelay (10);
    }

    printf("\nPlease select DDR CL:(0)3 (1)4 (2)5 (3)6:");
    while (1) {
        config_key = 0;
		if (serial_tstc()) {
            config_key = serial_getc();
            if(config_key == 48) {
                config->CAS_latency = 3;
                break;
            } else if(config_key == 49) {
                config->CAS_latency = 4;
                break;
            } else if(config_key == 50) {
                config->CAS_latency = 5;
                break;
            } else if(config_key == 51) {
                config->CAS_latency = 6;
                break;
            } else {
                printf("Error DDR CL\n");
                printf("\nPlease select DDR CL:(0)3 (1)4 (2)5 (3)6:");
            }
            //printf("\n");
        }
        udelay (10);
    }
}
#endif

void ddr_init(void)
{
struct athr_ddr_init_config default_ddr2_config = {
    .CAS_latency    = 6,
    .write_recovery = 7,
    .burst_length   = DDR2_BURSTLENGTH_4,
    .burst_type     = DDR_BURSTTYPE_SEQUENTIAL,
    .ddr_mode       = DDR2_MODE,
#ifdef QFN_TEST_BOARD_8BIT_DDR2
    .lane_number    = DDR_ALN_ONE_LANE,
#else
    .lane_number    = DDR_ALN_TWO_LANE,
#endif
    .density        = DDR_2GB,
    .trrd           = 5,
    .trp            = 7,
    .trc            = 0x18,
    .trcd           = 7,
    .tras           = 0x14,
    .twtr           = 5,
    .trtw           = 1,
    .trtp           = 4,
    .trfc           = 0x5a,
    .tmod           = 15,
#ifdef QFN_TEST_BOARD_8BIT_DDR2
     /*QFN test board: clk:300 odt:50*/
    .odt            = 0,  //0, 50, 75, 150
    .clock          = 200,
#else
     /*BGA demo board: clk:400 odt:75*/
     /*BGA test board: clk:300 odt:50*/
    .odt            = 0,  //0, 50, 75, 150
    .clock          = 200,
#endif
};
#ifdef DEBUG_DDR_CONFIG
    ddr_init_cfg_input(&default_ddr2_config);
    printf("\n###ddr clock:%d odt:%d cl:%d###\n",
                                    default_ddr2_config.clock,
                                    default_ddr2_config.odt,
                                    default_ddr2_config.CAS_latency);
#else
    printf(" ddr clock:%d\t", default_ddr2_config.clock);
#endif
    set_ddr_speed(default_ddr2_config.clock);
    athr_ddr_config_dump(default_ddr2_config);
    athr_ddr2_config_init(default_ddr2_config);

    athr_ddr_init_datatrain(default_ddr2_config);

}
