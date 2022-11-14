#include "ddr.h"


int32_t athr_ddr3_config_init(struct athr_ddr_init_config athr_ddr_init_val)
{
    int32_t ret_val = 0;
    uint32_t odt_reg_val = 0;
    union athr_ddr_timer_reg                 athr_ddr_timer_init;
    union athr_ddr_ctrl_misc_reg             athr_ddr_ctrl_misc_init;
    union athr_ddr_cfg_reg                   athr_ddr_cfg_init;
    union athr_ddr_extend_mode_register3_reg athr_ddr_extend_mode_register3_init;
    union athr_ddr_extend_mode_register2_reg athr_ddr_extend_mode_register2_init;
    union athr_ddr_extend_mode_register1_reg athr_ddr_extend_mode_register1_init;
    union athr_ddr3_mode_register_reg         athr_ddr_mode_register_init;
    union athr_ddr_config2_reg               athr_ddr_config2_init;
    union athr_ddr_config1_reg               athr_ddr_config1_init;
    union athr_ddr_config0_reg               athr_ddr_config0_init;
    union athr_ddr_refresh_reg              athr_ddr_refresh_init;
    union athr_ddr_phy_rd_ctrl0_reg         phy_rd_ctrl0_init;
    union athr_ddr_phy_rd_ctrl1_reg         phy_rd_ctrl1_init;
    union athr_ddr_phy_misc_reg             phy_misc_init;
    union athr_dqs_gate_phase_reg           dqs_gate_phase_init;
	union athr_ddr2_config_reg 				ddr2_config_init;

    memset(&athr_ddr_timer_init,                0, sizeof(athr_ddr_timer_init));
    memset(&athr_ddr_ctrl_misc_init,            0, sizeof(athr_ddr_ctrl_misc_init));
    memset(&athr_ddr_cfg_init,                  0, sizeof(athr_ddr_cfg_init));
    memset(&athr_ddr_extend_mode_register3_init,0, sizeof(athr_ddr_extend_mode_register3_init));
    memset(&athr_ddr_extend_mode_register2_init,0, sizeof(athr_ddr_extend_mode_register2_init));
    memset(&athr_ddr_extend_mode_register1_init,0, sizeof(athr_ddr_extend_mode_register1_init));
    memset(&athr_ddr_mode_register_init,        0, sizeof(athr_ddr_mode_register_init));
    memset(&athr_ddr_config2_init,              0, sizeof(athr_ddr_config2_init));
    memset(&athr_ddr_config1_init,              0, sizeof(athr_ddr_config1_init));
    memset(&athr_ddr_config0_init,              0, sizeof(athr_ddr_config0_init));
    memset(&athr_ddr_refresh_init,              0, sizeof(athr_ddr_refresh_init));
    memset(&phy_rd_ctrl0_init,                  0, sizeof(phy_rd_ctrl0_init));
    memset(&phy_rd_ctrl1_init,                  0, sizeof(phy_rd_ctrl1_init));
    memset(&dqs_gate_phase_init,                0, sizeof(dqs_gate_phase_init));
    memset(&phy_misc_init,                      0, sizeof(phy_misc_init));
    memset(&ddr2_config_init,                        0, sizeof(ddr2_config_init));

    if(athr_ddr_init_val.odt == 0) {
        odt_reg_val = 0;
    } else if(athr_ddr_init_val.odt == 20){
        odt_reg_val = (1 << 9);
    } else if(athr_ddr_init_val.odt == 30){
        odt_reg_val = (1 << 9) | (1 << 2);
    } else if(athr_ddr_init_val.odt == 40){
        odt_reg_val = (1 << 6) | (1 << 2);
    } else if(athr_ddr_init_val.odt == 60){
        odt_reg_val = (1 << 2);
    } else if(athr_ddr_init_val.odt == 120){
        odt_reg_val = (1 << 6);
    }
    /* Apply power with RST# asserted,
     * wait for 200us at least
       step 1 waits 200us: 200us/5ns= 40000 */
    ddr_timer_delay(40000);

    /* Deassert RST# signal, wait for 500us at least,   *
     * assert CKE. Before CKE goes high, CK needs to be *
     * stable for at least 10ns or 5 cycles             *
     */
    /* step 3 wait 500 us cycles: 500us/5ns= 100000 */
    ddr_timer_delay(100000);

    athr_ddr_ctrl_misc_init.val = music_reg_rd(DDR_CTRL_MISC_REG);
    athr_ddr_ctrl_misc_init.content.ALN      = athr_ddr_init_val.lane_number;
    athr_ddr_ctrl_misc_init.content.DDR_MODE = athr_ddr_init_val.ddr_mode;
    athr_ddr_ctrl_misc_init.content.DENSITY  = athr_ddr_init_val.density;
    athr_ddr_ctrl_misc_init.content.ADDR_MAP_MODE = athr_ddr_init_val.addr_map_mode;
    athr_ddr_ctrl_misc_init.content.CKE = 0;
    music_reg_wr_nf(DDR_CTRL_MISC_REG, athr_ddr_ctrl_misc_init.val);

    DDR_DEBUG("0:addr = 0x%x, val = 0x%x\r\n", DDR_CTRL_MISC_REG, athr_ddr_ctrl_misc_init.val);

    /* DDR MR register WR value must be the same with the tWR value in the DDR_Config1
       DDR MR register burst_type & burst_length should set the same with DDR_Config1 register
       CL bit value should set the same in DDR_config0 register and DDR_MR register
     */

    athr_ddr_config0_init.val = music_reg_rd(DDR_CONFIG0_REG);
    athr_ddr_config0_init.content.CL   = athr_ddr_init_val.CAS_latency;
    athr_ddr_config0_init.content.TRRD = athr_ddr_init_val.trrd;
    athr_ddr_config0_init.content.TRP  = athr_ddr_init_val.trp;
    athr_ddr_config0_init.content.TRC  = athr_ddr_init_val.trc;
    athr_ddr_config0_init.content.TRCD = athr_ddr_init_val.trcd;
    athr_ddr_config0_init.content.TRAS = athr_ddr_init_val.tras;
    music_reg_wr_nf(DDR_CONFIG0_REG, athr_ddr_config0_init.val);

    DDR_DEBUG("0:addr = 0x%x, val = 0x%x\r\n", DDR_CONFIG0_REG,
        athr_ddr_config0_init.val);

    athr_ddr_config1_init.val = music_reg_rd(DDR_CONFIG1_REG);
    athr_ddr_config1_init.content.TWTR = athr_ddr_init_val.twtr;
    athr_ddr_config1_init.content.TRTP = athr_ddr_init_val.trtp;
    athr_ddr_config1_init.content.TRTW = athr_ddr_init_val.trtw;
    athr_ddr_config1_init.content.TWR  = athr_ddr_init_val.write_recovery;
    athr_ddr_config1_init.content.BURST_TYPE = athr_ddr_init_val.burst_type;
    if(athr_ddr_init_val.burst_length == DDR2_BURSTLENGTH_4) {
        athr_ddr_config1_init.content.BURST_LENGTH = 4;
    } else if (athr_ddr_init_val.burst_length == DDR2_BURSTLENGTH_8){
        athr_ddr_config1_init.content.BURST_LENGTH = 8;
    }
    music_reg_wr_nf(DDR_CONFIG1_REG, athr_ddr_config1_init.val);

    DDR_DEBUG("0:addr = 0x%x, val = 0x%x\r\n", DDR_CONFIG1_REG,
        athr_ddr_config1_init.val);

    athr_ddr_config2_init.val = music_reg_rd(DDR_CONFIG2_REG);
    athr_ddr_config2_init.content.TRFC = athr_ddr_init_val.trfc;
    athr_ddr_config2_init.content.TMRD = athr_ddr_init_val.tmrd;

    music_reg_wr_nf(DDR_CONFIG2_REG, athr_ddr_config2_init.val);

    DDR_DEBUG("0:addr = 0x%x, val = 0x%x\r\n", DDR_CONFIG2_REG,
        athr_ddr_config2_init.val);

    athr_ddr_refresh_init.val = music_reg_rd(DDR_REFRESH_REG);
    athr_ddr_refresh_init.content.REF_BURST = 7;
    athr_ddr_refresh_init.content.REF_EN = 1;
    athr_ddr_refresh_init.content.REF_PERIOD = 0x25e0;
        /* (615* athr_ddr_init_val.clock * 19) / 200; */

    music_reg_wr_nf(DDR_REFRESH_REG, athr_ddr_refresh_init.val);
    DDR_DEBUG("0:addr = 0x%x, val = 0x%x\r\n", DDR_REFRESH_REG,
        athr_ddr_refresh_init.val);


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
    music_reg_wr_nf(DQS_GATE_PHASE_REG, dqs_gate_phase_init.val);
    DDR_DEBUG("0: addr = 0x%x val = 0x%x\n", DQS_GATE_PHASE_REG,
        dqs_gate_phase_init.val);

    phy_rd_ctrl0_init.content.DQS_IN_EDLY_CODE_OFF1 =
        DEFAULT_DQS_IN_EDLY_CODE_OFF;
    phy_rd_ctrl0_init.content.DQS_IN_EDLY_CODE_OFF0 =
        DEFAULT_DQS_IN_EDLY_CODE_OFF;
    music_reg_wr_nf(DDR_PHY_RD_CTRL0_REG, phy_rd_ctrl0_init.val);
    DDR_DEBUG("0: addr = 0x%x val = 0x%x\n", DDR_PHY_RD_CTRL0_REG,
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
    DDR_DEBUG("0: addr = 0x%x val = 0x%x \n", DDR_PHY_RD_CTRL1_REG,
        phy_rd_ctrl1_init.val);

    phy_misc_init.content.ISERDES_FLUSH = 1;
    phy_misc_init.content.CODE_SET_MANU_EN = 1;
    phy_misc_init.content.CNTL_OE_EN = 1;
    music_reg_wr_nf(DDR_PHY_MISC_REG, phy_misc_init.val);
    DDR_DEBUG("0: addr 0x%x val 0x%x \n", DDR_PHY_MISC_REG,
        phy_misc_init.val);


#if 0
    /* Apply power with RST# asserted,
     * wait for 200us at least
       step 1 waits 200us: 200us/5ns= 40000 */
    ddr_timer_delay(40000);

    /* Deassert RST# signal, wait for 500us at least,   *
     * assert CKE. Before CKE goes high, CK needs to be *
     * stable for at least 10ns or 5 cycles             *
     */
    /* step 3 wait 500 us cycles: 500us/5ns= 100000 */
    ddr_timer_delay(100000);

#endif
    /* Wait for tXPR time before MRS */
    athr_ddr_ctrl_misc_init.content.CKE = 1;
    music_reg_wr_nf(DDR_CTRL_MISC_REG, athr_ddr_ctrl_misc_init.val);

    athr_ddr_extend_mode_register2_init.content.EMR2S = 0;
    athr_ddr_extend_mode_register2_init.content.EMR2_VALUE = DDR2_EMR2VAL;
    /* 4 Issue a load Extended Mode Register 2 command */
    music_reg_wr_nf(DDR_EXTENDED_MODE_REG2,
        athr_ddr_extend_mode_register2_init.val);

    DDR_DEBUG("4:addr = 0x%x, val = 0x%x\r\n", DDR_EXTENDED_MODE_REG2,
        athr_ddr_extend_mode_register2_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.BA = 2;
    athr_ddr_cfg_init.content.INIT_START = 1;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("4:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 5 Issue a load Extended Mode Register 3 command */
    athr_ddr_extend_mode_register3_init.content.EMR3S = 0;
    athr_ddr_extend_mode_register3_init.content.EMR3_VALUE = DDR2_EMR3VAL;
    music_reg_wr_nf(DDR_EXTENDED_MODE_REG3,
        athr_ddr_extend_mode_register3_init.val);

    DDR_DEBUG("5:addr = 0x%x, val = 0x%x\r\n", DDR_EXTENDED_MODE_REG3,
        athr_ddr_extend_mode_register3_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.BA = 3;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("5:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 6 Issue a load Extended Mode Register 1 command to enable DLL */
    athr_ddr_extend_mode_register1_init.content.EMR1S = 0;
    athr_ddr_extend_mode_register1_init.content.EMR1_VALUE = DDR2_EMR1VAL0 | odt_reg_val;
    music_reg_wr_nf(DDR_EXTENDED_MODE_REG1,
        athr_ddr_extend_mode_register1_init.val);

    DDR_DEBUG("6:addr = 0x%x, val = 0x%x\r\n", DDR_EXTENDED_MODE_REG1,
        athr_ddr_extend_mode_register1_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.BA = 1;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);
    DDR_DEBUG("6:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);
    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 7 Issue a load Mode Register command to reset DLL */
#if 0
    athr_ddr_mode_register_init.val = 0x721;
#endif
    athr_ddr_mode_register_init.content.DLL = 1;
    athr_ddr_mode_register_init.content.PD = 0;
    athr_ddr_mode_register_init.content.BT = athr_ddr_init_val.burst_type;
    if(athr_ddr_init_val.CAS_latency >= 12 &&
            athr_ddr_init_val.CAS_latency <= 13) {
        athr_ddr_mode_register_init.content.CL0 = 1;
        athr_ddr_mode_register_init.content.CL1 =
            athr_ddr_init_val.CAS_latency - 12;
    } else if(athr_ddr_init_val.CAS_latency >= 5 &&
            athr_ddr_init_val.CAS_latency < 12){
        athr_ddr_mode_register_init.content.CL0 = 0;
        athr_ddr_mode_register_init.content.CL1 =
            athr_ddr_init_val.CAS_latency - 4;
    } else {
        return -1;
    }
    athr_ddr_mode_register_init.content.BURSTLENGTH = 1;

    if(athr_ddr_init_val.write_recovery >= 5 &&
            athr_ddr_init_val.write_recovery <= 8) {
        athr_ddr_mode_register_init.content.WR =
            athr_ddr_init_val.write_recovery - 4;
    } else if (athr_ddr_init_val.write_recovery == 10) {
        athr_ddr_mode_register_init.content.WR = 5;
    } else if (athr_ddr_init_val.write_recovery == 12) {
        athr_ddr_mode_register_init.content.WR = 6;
    } else if (athr_ddr_init_val.write_recovery == 14) {
        athr_ddr_mode_register_init.content.WR = 7;
    } else if (athr_ddr_init_val.write_recovery == 16) {
        athr_ddr_mode_register_init.content.WR = 0;
    }
    music_reg_wr_nf(DDR_MODE_REGISTER_REG,
        athr_ddr_mode_register_init.val);

    DDR_DEBUG("7:addr = 0x%x, val = 0x%x\r\n", DDR_MODE_REGISTER_REG,
        athr_ddr_mode_register_init.val);

    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.BA = 0;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;
    athr_ddr_cfg_init.content.CMD = CMD_LOADMODE;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("7:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /*wait 512 tCK for ddr device DLL lock */
    ddr_timer_delay(256);

    /* 8 Issue ZQCL command to start ZQ calibration */
    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.CMD = CMD_ZQCL;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;

    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("8:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 9 Wait for both tDLLK and tZQinit complete */
    ddr_timer_delay(10000);

    /* 9.1 add precharge all*/
    athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    athr_ddr_cfg_init.content.CMD = CMD_PRECHARGE_ALL;
    athr_ddr_cfg_init.content.CMD_ISSUE = 1;

    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    do {
        athr_ddr_cfg_init.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_init.content.CMD_ISSUE);

    /* 10 set init done reg */
    memset(&athr_ddr_cfg_init.val, 0, sizeof(athr_ddr_cfg_init));

    athr_ddr_cfg_init.content.INIT_START = 0;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("10:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

    /* 10 set init done reg */
    memset(&athr_ddr_cfg_init.val, 0, sizeof(athr_ddr_cfg_init));

    athr_ddr_cfg_init.content.INIT_DONE = 1;
    music_reg_wr_nf(DDR_CFG_REG, athr_ddr_cfg_init.val);

    DDR_DEBUG("10:addr = 0x%x, val = 0x%x\r\n", DDR_CFG_REG,
        athr_ddr_cfg_init.val);

	if(MUSIC_IS_VIVO_B0()) {
		ddr2_config_init.val = music_reg_rd(DDR_DDR2_CONFIG_REG);
		ddr2_config_init.content.DDR2_ODT = 1;
		music_reg_wr_nf(DDR_DDR2_CONFIG_REG, ddr2_config_init.val);
	}

    return ret_val;
}

//#define DEBUG_DDR_CONFIG

#ifdef DEBUG_DDR_CONFIG
void ddr_init_cfg_input(struct athr_ddr_init_config *config)
{

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
                printf("\nPlease select DDR clock:(0)100 (1)200"
                    " (2)266 (3)300 (4)400:");
            }
        }
        udelay (10);
    }

    printf("\nPlease select DDR bit number:(0)8 (1)16:");
    while (1) {
        config_key = 0;
		if (serial_tstc()) {
            config_key = serial_getc();
            if(config_key == 48) {
                config->lane_number = DDR_ALN_ONE_LANE;
                break;
            } else if(config_key == 49) {
                config->lane_number = DDR_ALN_TWO_LANE;
                break;
            } else {
                printf("Error DDR clock\n");
                printf("\nPlease select DDR bit number:(0)8 (1)16: ");
            }
        }
        udelay (10);
    }

    printf("\nPlease select DDR odt:(0)0 (1)20 (2)30 (3)40 (4)60 (5)120: ");
    while (1) {
        config_key = 0;
		if (serial_tstc()) {
            config_key = serial_getc();
            if(config_key == 48) {
                config->odt = 0;
                break;
            } else if(config_key == 49) {
                config->odt = 20;
                break;
            } else if(config_key == 50) {
                config->odt = 30;
                break;
            } else if(config_key == 51) {
                config->odt = 40;
                break;
            } else if(config_key == 52) {
                config->odt = 60;
                break;
            } else if(config_key == 53) {
                config->odt = 120;
                break;
            } else {
                printf("Error DDR odt\n");
                printf("\nPlease select DDR odt:(0)0"
                    " (1)20 (2)30 (3)40 (4)60 (5)120:");
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
        }
        udelay (10);
    }
}
#endif
void ddr_init(void)
{
    printf("ddr3 init\n");
#ifdef DDR3_WB_1GB_CONFIG
struct athr_ddr_init_config default_ddr3_config = {
    .CAS_latency    = 6,	/* support CL 6/7/8/9/11/13 */
    .write_recovery = 10, 	/* 15nS */
    .burst_length   = DDR2_BURSTLENGTH_8,
    .burst_type     = DDR_BURSTTYPE_SEQUENTIAL,
    .ddr_mode       = DDR3_MODE,
    .trrd           = 4,	/* max(4nCK, 6nS) = 10nS */
    .trp            = 6, 	/* 13.5nS ->(6nCK * 2.5nS) */
    .trc            = 0x14,	/* 49.5nS -> 20 nCK */
    .trcd           = 0x6,	/* 13.5nS -> 6nCK */
    .tras           = 0x0f, /* 36nS -> 15nCK */
    .twtr           = 4,	/* max(4nCK, 7.5nS) = 4nCK */
    .trtp           = 4,	/* max(4nCK, 7.5nS) = 4nCK */
    .trtw           = 1,	/* ?? */
    .trfc           = 0x33,	/* 127.5/2.5 = 51 */
    .tmrd           = 3,	/* 4nCK, but vaild value should be 2~3?? */
    .lane_number    = DDR_ALN_ONE_LANE,
    .density        = DDR_1GB,
    .clock          = 400,
	.odt 			= 120,
};
#else
struct athr_ddr_init_config default_ddr3_config = {
    .CAS_latency    = 6, /*3, 4, 5, 6*/
    .write_recovery = 7,
    .burst_length   = DDR2_BURSTLENGTH_8, /*2:BURSTLENGTH_4, 3:BURSTLENGTH_8*/
    .burst_type     = DDR_BURSTTYPE_SEQUENTIAL,
    .ddr_mode       = DDR3_MODE,
    .trrd           = 5,
    .trp            = 7,
    .trc            = 0x1b,
    .trcd           = 0x7,
    .tras           = 0x14,
    .twtr           = 4,
    .trtp           = 4,
    .trtw           = 1,
    .trfc           = 0x6b,
    .tmrd           = 2,
    .lane_number    = DDR_ALN_TWO_LANE,/*0:ONE_LANE,1:TWO_LANE, 3:FOUR_LANE*/
    .density        = DDR_2GB,
    .clock          = 300, /*100, 200, 266, 300, 400*/
};
#endif

#ifdef DEBUG_DDR_CONFIG
    ddr_init_cfg_input(&default_ddr3_config);
#endif
    set_ddr_speed(default_ddr3_config.clock);
    athr_ddr_config_dump(default_ddr3_config);
    athr_ddr3_config_init(default_ddr3_config);
    athr_ddr_init_datatrain(default_ddr3_config);
}
