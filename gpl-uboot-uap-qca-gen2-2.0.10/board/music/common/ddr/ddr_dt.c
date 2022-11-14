#include "ddr.h"

#if 1
#define DDR_DT_DQS_MAXNUM               4
#define DDR_DT_DATA_MAXNUM              4
#define DDR_DT_DLY_MAXNUM               0x40
//#define DDR_I_SERDES_FLUSH_TIME         0x200
#endif
#define DDR_READ_WAIT_TIME              0x2ff

static int32_t
athr_dt_write_ddr_issue(uint32_t wr_dt_addr);
#if 0
uint32_t
ddr_dt_pattern[] = {0xaaaaa5a5, 0x55555a5a, 0xaaaaa5a5, 0x55555a5a};
#endif
int32_t set_ddr_speed(uint32_t ddr_speed)
{
    uint32_t clock_ctrl_reg = 0;
    uint32_t speed_reg_val = 0;

    clock_ctrl_reg = music_reg_rd(GLB_CLOCK_CTRL_REG);
    clock_ctrl_reg &= ~(DDR_SPEED_MODE_MASK);
    clock_ctrl_reg &= ~(1 << 27);

    if (ddr_speed == 100) {
        speed_reg_val = DDR_100M;
    } else if (ddr_speed == 133) {
        speed_reg_val = DDR_133M;
    } else if (ddr_speed == 200) {
        speed_reg_val = DDR_200M;
    } else if (ddr_speed == 266) {
        speed_reg_val = DDR_266M;
    } else if (ddr_speed == 300) {
        speed_reg_val = DDR_300M;
    } else if (ddr_speed  == 400) {
        speed_reg_val = DDR_400M;
    } else if (ddr_speed == 480) {
        speed_reg_val = DDR_480M;
    }

    clock_ctrl_reg |= (speed_reg_val << DDR_SPEED_MODE_SHIFT);
    music_reg_wr_nf(GLB_CLOCK_CTRL_REG, clock_ctrl_reg);

    return 0;
}

int32_t get_ddr_speed(uint32_t * ddr_speed)
{
    uint32_t clock_ctrl_reg = 0;

    clock_ctrl_reg = music_reg_rd(GLB_CLOCK_CTRL_REG);
    clock_ctrl_reg &= DDR_SPEED_MODE_MASK;
    clock_ctrl_reg >>= DDR_SPEED_MODE_SHIFT;

    if(0 == clock_ctrl_reg) {
        * ddr_speed = 100;
    } else if (1 == clock_ctrl_reg) {
        * ddr_speed = 133;
    } else if (2 == clock_ctrl_reg) {
        * ddr_speed = 200;
    } else if (3 == clock_ctrl_reg) {
        * ddr_speed = 266;
    } else if (4 == clock_ctrl_reg) {
        * ddr_speed = 300;
    } else if (5 == clock_ctrl_reg) {
        * ddr_speed = 400;
    } else if (6 == clock_ctrl_reg) {
        * ddr_speed = 480;
    } else {
        return -1;
    }
    return 0;
}

int32_t ddr_timer_delay(uint32_t delay)
{
    union  athr_ddr_timer_reg       athr_timer_delay;
    union  athr_ddr_ctrl_misc_reg   athr_timer_done;
    uint32_t        ddr_speed = 0;

    memset(& athr_timer_delay, 0, sizeof(athr_timer_delay));
    memset(& athr_timer_done, 0, sizeof(athr_timer_done));
    get_ddr_speed(& ddr_speed);

    athr_timer_delay.content.COUNTER = delay * ddr_speed;
    music_reg_wr_nf(DDR_TIMER_REG, athr_timer_delay.val);

    do {
        athr_timer_done.val = music_reg_rd(DDR_CTRL_MISC_REG);
    }while(!(athr_timer_done.content.TIMER_DONE));

    return 0;
}

static int32_t
athr_dt_read_ddr(uint32_t * rd_dt_data, int count, uint32_t rd_dt_addr)
{
    uint32_t loop =0;

    union   athr_ddr_cfg_reg          athr_ddr_cfg_dt;
    union   athr_ddr_phy_misc_reg athr_dt_phy_misc_ctrl;
    union   athr_ddr_ctrl_misc_reg  athr_dt_misc;

    memset(& athr_dt_phy_misc_ctrl,
        0, sizeof(athr_dt_phy_misc_ctrl));
    memset(& athr_ddr_cfg_dt,
        0, sizeof(athr_ddr_cfg_dt));
    memset(& athr_dt_misc,
        0, sizeof(athr_dt_misc));

    /* reset the DT register because it will be used
        to store read data from ddr sdram */
    music_reg_wr_nf(DDR_DT0_REG, 0xffffffff);
    music_reg_wr_nf(DDR_DT1_REG, 0xffffffff);
    music_reg_wr_nf(DDR_DT2_REG, 0xffffffff);
    music_reg_wr_nf(DDR_DT3_REG, 0xffffffff);

    /* set read ddr address	*/
    music_reg_wr_nf(DDR_DT_ADDR_REG, rd_dt_addr);

    /* CMD_ISSUE=1, DT_START=1, CMD=9;  read ddr command */
    music_reg_wr_nf(DDR_CFG_REG, 0x81500000);
    music_reg_wr_nf(DDR_TIMER_REG, DDR_READ_WAIT_TIME);

    /* wait read ddr command issue */
    do {
        athr_ddr_cfg_dt.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_dt.content.CMD_ISSUE);

    /* wait DDR_TIMER timer done and RDATA valid */
    do {
        athr_ddr_cfg_dt.val = music_reg_rd(DDR_CFG_REG);
	    athr_dt_misc.val = music_reg_rd(DDR_CTRL_MISC_REG);
    }while((1!=athr_ddr_cfg_dt.content.RD_DATA_VLD)
            && (1!=athr_dt_misc.content.TIMER_DONE));

    /* iserdes flush issue */
    athr_dt_phy_misc_ctrl.val = music_reg_rd(DDR_PHY_MISC_REG);
    athr_dt_phy_misc_ctrl.content.ISERDES_FLUSH = 1;
    music_reg_wr_nf(DDR_PHY_MISC_REG, athr_dt_phy_misc_ctrl.val);

    ddr_timer_delay(DDR_I_SERDES_FLUSH_TIME);

    for(loop = 0; loop < count; loop ++) {
        rd_dt_data[loop] = music_reg_rd(DDR_DT0_REG +
            loop * DDR_REG_WIDTH);

    }
    /* when aln = 0, burst_length = 8 we need
       compare rd_data_data[0] rd_data_data[2]
       with   ddr_dt_pattern[0], ddr_dt_pattern[2]*/

    return 0;
}

static int32_t
athr_dt_write_ddr(uint32_t * ddr_dt_pattern, uint32_t wr_dt_addr, int aln_number, int burst_length)
{
    union       athr_ddr_cfg_reg        athr_ddr_cfg_dt;

    memset(& athr_ddr_cfg_dt, 0, sizeof(athr_ddr_cfg_dt));

    /* write ddr configuration for ddr data training, ddr3
       sdram only be setted BURSTLENGTH as 4, but in the training process,
       ddr controller set BURSTLENGTH as 8; after training finished,
       then set ddr controller BURSTLENGTH as 4 */
    if (aln_number == 0 && burst_length == DDR2_BURSTLENGTH_4) {
        music_reg_wr_nf(DDR_DT0_REG, 0x5a55a5aa);
        athr_dt_write_ddr_issue(wr_dt_addr);
	    ddr_dt_pattern[0] = 0x5a55a5aa;
        ddr_dt_pattern[1] = 0x00000000;
        ddr_dt_pattern[2] = 0x00000000;
        ddr_dt_pattern[3] = 0x00000000;
    }
    else if (aln_number == 1 && burst_length == DDR2_BURSTLENGTH_4) {
        music_reg_wr_nf(DDR_DT0_REG, 0x55555a5a);
        music_reg_wr_nf(DDR_DT1_REG, 0xaaaaa5a5);
        athr_dt_write_ddr_issue(wr_dt_addr);
        ddr_dt_pattern[0] = 0x55555a5a;
        ddr_dt_pattern[1] = 0xaaaaa5a5;
        ddr_dt_pattern[2] = 0x00000000;
        ddr_dt_pattern[3] = 0x00000000;
    }
    else if (aln_number == 0 && burst_length == DDR2_BURSTLENGTH_8) {
        music_reg_wr_nf(DDR_DT0_REG, 0x5a55a5aa);
        athr_dt_write_ddr_issue(wr_dt_addr);
        music_reg_wr_nf(DDR_DT0_REG, 0xa5aa5a55);
        athr_dt_write_ddr_issue(wr_dt_addr + 4);
        ddr_dt_pattern[0] = 0x5a55a5aa;
        ddr_dt_pattern[1] = 0x00000000;
        ddr_dt_pattern[2] = 0xa5aa5a55;
        ddr_dt_pattern[3] = 0x00000000;
    }
    else if (aln_number == 1 && burst_length == DDR2_BURSTLENGTH_8) {
        music_reg_wr_nf(DDR_DT0_REG, 0x55555a5a);
        music_reg_wr_nf(DDR_DT1_REG, 0xaaaaa5a5);
        athr_dt_write_ddr_issue(wr_dt_addr);
	    music_reg_wr_nf(DDR_DT0_REG, 0x55555a5a);
        music_reg_wr_nf(DDR_DT1_REG, 0xaaaaa5a5);
        athr_dt_write_ddr_issue(wr_dt_addr + 8);
        ddr_dt_pattern[0] = 0x55555a5a;
        ddr_dt_pattern[1] = 0xaaaaa5a5;
        ddr_dt_pattern[2] = 0x55555a5a;
        ddr_dt_pattern[3] = 0xaaaaa5a5;
    }

    return 0;
}

static int32_t
athr_dt_write_ddr_issue(uint32_t wr_dt_addr)
{
    union       athr_ddr_cfg_reg        athr_ddr_cfg_dt;

    memset(& athr_ddr_cfg_dt, 0, sizeof(athr_ddr_cfg_dt));

    music_reg_wr_nf(DDR_DT_ADDR_REG, wr_dt_addr);

    /* CMD_ISSUE=1, DT_START=1, CMD=8 */
    music_reg_wr_nf(DDR_CFG_REG, 0x81400000);

    do {
        athr_ddr_cfg_dt.val = music_reg_rd(DDR_CFG_REG);
    }while(athr_ddr_cfg_dt.content.CMD_ISSUE);

    return 0;
}


static uint32_t athr_dt_data_check(uint32_t * check_result, uint32_t * ddr_dt_rdata,
    uint32_t *ddr_dt_pattern, int32_t aln_number, uint32_t burst_length, int edge_type)
{
    uint32_t    dt_num_mask = 0;

    * check_result = 0;
    if((RDLY_EDGE == edge_type) && (aln_number == DDR_ALN_ONE_LANE)) {
        dt_num_mask = 0x00ff00ff;
    }
    else if((RDLY_EDGE == edge_type) && (aln_number == DDR_ALN_TWO_LANE)) {
        dt_num_mask = 0x0000ffff;
    }
    else if((FDLY_EDGE == edge_type) && (aln_number == DDR_ALN_ONE_LANE)) {
        dt_num_mask = 0xff00ff00;
    }
    else if((FDLY_EDGE == edge_type) && (aln_number == DDR_ALN_TWO_LANE)) {
        dt_num_mask = 0xffff0000;
    }

    if (aln_number == 0 && burst_length == DDR2_BURSTLENGTH_4) {
        if ((ddr_dt_pattern[0] & dt_num_mask) == (ddr_dt_rdata[0] &dt_num_mask)) {
	        * check_result = 1;
        } else {
            * check_result = 0;
	    }
    }
    else if (aln_number == 1 && burst_length == DDR2_BURSTLENGTH_4) {
        if (((ddr_dt_pattern[0] & dt_num_mask) == (ddr_dt_rdata[0] &dt_num_mask)) &&
	        ((ddr_dt_pattern[1] & dt_num_mask) == (ddr_dt_rdata[1] &dt_num_mask))) {
	        * check_result = 1;
        } else {
            * check_result = 0;
	    }
    }
    else if (aln_number == 0 && burst_length == DDR2_BURSTLENGTH_8) {
        if (((ddr_dt_pattern[0] & dt_num_mask) == (ddr_dt_rdata[0] &dt_num_mask)) &&
	        ((ddr_dt_pattern[2] & dt_num_mask) == (ddr_dt_rdata[2] &dt_num_mask))) {
	        * check_result = 1;
        } else {
            * check_result = 0;
	    }
    }
    else if (aln_number == 1 && burst_length == DDR2_BURSTLENGTH_8) {
        if (((ddr_dt_pattern[0] & dt_num_mask) == (ddr_dt_rdata[0] &dt_num_mask)) &&
	      ((ddr_dt_pattern[1] & dt_num_mask) == (ddr_dt_rdata[1] &dt_num_mask)) &&
	      ((ddr_dt_pattern[2] & dt_num_mask) == (ddr_dt_rdata[2] &dt_num_mask)) &&
	      ((ddr_dt_pattern[3] & dt_num_mask) == (ddr_dt_rdata[3] &dt_num_mask))) {
	        * check_result = 1;
        } else {
            * check_result = 0;
	    }
    }

    DDR_DEBUG("DT: check_result = 0x%x \r\n", * check_result);

    return 0;
}

int athr_ddr_init_datatrain(struct athr_ddr_init_config athr_ddr_init_val)
{
    uint32_t ddr_dt_qds_loop        = 0;
    uint32_t ddr_lane_loop          = 0;
    uint32_t ddr_dly_loop           = 0;
    uint32_t ddr_dly_type_loop      = 0;
    uint32_t check_result           = 0;
    uint32_t check_result_record    = 0;
    uint32_t ddr_dly_high_thr       = 0;
    uint32_t ddr_dly_low_thr        = 0;
    uint32_t rdly_val               = 0;
    uint32_t rdly_val_record        = 0;
    uint32_t fdly_val               = 0;
    uint32_t fdly_val_record        = 0;
    uint32_t edly_val               = 0;
    uint32_t edly_val_record        = 0;
    uint32_t dqs_in_gate_val_record = 0;
    uint32_t rdly_range             = 0;
    uint32_t rdly_range_record      = 0;
    uint32_t fdly_range             = 0;
    uint32_t fdly_range_record      = 0;
    uint32_t ddr_dt_lane_maxnum     = 0;
    uint32_t wr_dt_addr             = 0;
    uint32_t rd_dt_data[4];
    uint32_t ddr_dt_pattern[] =
        {
            0xaaaaa5a5, 0x55555a5a,
            0xaaaaa5a5, 0x55555a5a
        };


    union athr_ddr_phy_misc_reg         athr_dt_phy_misc;
    union athr_dqs_gate_phase_reg       athr_dt_dqs_gate_phase;
    union athr_ddr_phy_rd_ctrl0_reg     athr_dt_phy_ctrl0;
    union athr_ddr_phy_rd_ctrl1_reg     athr_dt_phy_ctrl1;
    union athr_ddr_ctrl_misc_reg        athr_dt_misc;
    union athr_ddr_cfg_reg              athr_dt_ddr_cfg;
    union athr_ddr_config1_reg          athr_dt_ddr_config1;

    memset(rd_dt_data,                  0, sizeof(rd_dt_data));
    memset(& athr_dt_misc,              0, sizeof(athr_dt_misc));
    memset(& athr_dt_ddr_cfg,           0, sizeof(athr_dt_ddr_cfg));
    memset(& athr_dt_phy_misc,          0, sizeof(athr_dt_phy_misc));
    memset(& athr_dt_dqs_gate_phase,    0, sizeof(athr_dt_dqs_gate_phase));
    memset(& athr_dt_phy_ctrl0,         0, sizeof(athr_dt_phy_ctrl0));
    memset(& athr_dt_phy_ctrl1,         0, sizeof(athr_dt_phy_ctrl1));
    memset(& athr_dt_ddr_config1,       0, sizeof(athr_dt_ddr_config1));

    /* set REOD_BP =1 for data training */
    athr_dt_misc.val = music_reg_rd(DDR_CTRL_MISC_REG);
    athr_dt_misc.content.REOD_BP = 1;
    music_reg_wr_nf(DDR_CTRL_MISC_REG, athr_dt_misc.val);

    DDR_DEBUG("DT: write reg 0x%x val 0x%x\n", DDR_CTRL_MISC_REG, athr_dt_misc.val);

    /* iserdes flush issue */
    athr_dt_phy_misc.val = music_reg_rd(DDR_PHY_MISC_REG);
    athr_dt_phy_misc.content.ISERDES_FLUSH = 1;
    athr_dt_phy_misc.content.CODE_SET_MANU_EN = 1;
    music_reg_wr_nf(DDR_PHY_MISC_REG, athr_dt_phy_misc.val);

    DDR_DEBUG("DT: write reg 0x%x val 0x%x \n", DDR_PHY_MISC_REG, athr_dt_phy_misc.val);

    /* wait iserdes flush done */
    ddr_timer_delay(DDR_I_SERDES_FLUSH_TIME);

    /* write golden training pattern to DT register, then write them to ddr sdram */
    athr_dt_write_ddr(ddr_dt_pattern, wr_dt_addr, athr_ddr_init_val.lane_number, athr_ddr_init_val.burst_length);

    /* get the maximum lane number according to ALN configuration */
    if(athr_ddr_init_val.lane_number == DDR_ALN_ONE_LANE) {
        ddr_dt_lane_maxnum = 1;
    } else if ((athr_ddr_init_val.lane_number == DDR_ALN_TWO_LANE)) {
        ddr_dt_lane_maxnum = 2;
    }

    /* data training loop based on each a ddr byte lane */
    for(ddr_lane_loop = 0; ddr_lane_loop < ddr_dt_lane_maxnum; ddr_lane_loop ++) {
        memset(& athr_dt_dqs_gate_phase, 0, sizeof(athr_dt_dqs_gate_phase));
        memset(& athr_dt_phy_ctrl0,     0, sizeof(athr_dt_phy_ctrl0));
        memset(& athr_dt_phy_ctrl1,     0, sizeof(athr_dt_phy_ctrl1));

        DDR_DEBUG("DT: ddr_lane_loop = %d\n", ddr_lane_loop);

        rdly_range = 0;
        fdly_range = 0;
        rdly_range_record = 0;
        fdly_range_record = 0;
        edly_val_record = 0;
        rdly_val_record = 0;
        fdly_val_record = 0;
        dqs_in_gate_val_record = 0;

        /* data training loop based on each a DQS_IN_GATE_PHASE  */
        /* DQS_IN_GATE sequence is 3-0-1-2*/
        ddr_dt_qds_loop = 3;

        while(1) {
		 athr_dt_dqs_gate_phase.val = music_reg_rd(DQS_GATE_PHASE_REG);
            athr_dt_phy_ctrl0.val = music_reg_rd(DDR_PHY_RD_CTRL0_REG);
            athr_dt_phy_ctrl1.val = music_reg_rd(DDR_PHY_RD_CTRL1_REG);

            DDR_DEBUG("DT: LANE%d = 0x%x \r\n", ddr_lane_loop,ddr_dt_qds_loop);

            switch(ddr_lane_loop) {
                case 0:
                    athr_dt_dqs_gate_phase.content.DQS_IN_GATE_PHASE_LANE0 = ddr_dt_qds_loop;
                    break;
                case 1:
                    athr_dt_dqs_gate_phase.content.DQS_IN_GATE_PHASE_LANE1 = ddr_dt_qds_loop;
                    break;
                default:
                    break;
            }

            music_reg_wr_nf(DQS_GATE_PHASE_REG, athr_dt_dqs_gate_phase.val);
            DDR_DEBUG("DT: write reg 0x%x val %x \r\n", DQS_GATE_PHASE_REG, athr_dt_dqs_gate_phase.val);

            /* data training loop based on each a DQS_IN delay code type:
                0: dqs_in rise edge delay code;
                1: dqs_in falling edge delay code; */
            for(ddr_dly_type_loop = 0; ddr_dly_type_loop < 2; ddr_dly_type_loop ++) {
                check_result        = 0;
                check_result_record = 0;
                ddr_dly_high_thr    = 0;
                ddr_dly_low_thr     = 0;
                if (1==ddr_dly_type_loop) {
                    switch(ddr_lane_loop) {
                    case 0:
                        athr_dt_phy_ctrl0.content.DQS_IN_EDLY_CODE_OFF0 = edly_val;
			            break;
                    case 1:
                        athr_dt_phy_ctrl0.content.DQS_IN_EDLY_CODE_OFF1 = edly_val;
			            break;
                    default:
                        break;
                    }
                }

                /* use rising delay code * 2 as extend falling edge delay code */
                music_reg_wr_nf(DDR_PHY_RD_CTRL0_REG, athr_dt_phy_ctrl0.val);

                /* data training loop based on each a DQS_IN delay code     */
                for(ddr_dly_loop = 0; ddr_dly_loop < DDR_DT_DLY_MAXNUM; ddr_dly_loop ++) {
                    if (0 == ddr_dly_type_loop) {
                        /*only for dqs_in rising edge delay code training */
                        switch(ddr_lane_loop) {
                            case 0:
                                athr_dt_phy_ctrl1.content.DQS_IN_RDLY_CODE_OFF0 = ddr_dly_loop;
                                break;
                            case 1:
                                athr_dt_phy_ctrl1.content.DQS_IN_RDLY_CODE_OFF1 = ddr_dly_loop;
                                break;
                            default:
                                break;
                        }
                    } else {
                        /* only for dqs_in falling edge delay code training */
                        switch(ddr_lane_loop) {
                            case 0:
                                athr_dt_phy_ctrl1.content.DQS_IN_FDLY_CODE_OFF0 = ddr_dly_loop;
                                break;
                            case 1:
                                athr_dt_phy_ctrl1.content.DQS_IN_FDLY_CODE_OFF1 = ddr_dly_loop;
                                break;
                            default:
                                break;
                        }
                    }

                    /* write dqs_in rising/falling edge delay code to register */
			        music_reg_wr_nf(DDR_PHY_RD_CTRL1_REG, athr_dt_phy_ctrl1.val);
                    DDR_DEBUG("DT: write reg 0x%x val %x\n", DDR_PHY_RD_CTRL1_REG, athr_dt_phy_ctrl1.val);

                    check_result_record = check_result;
			        athr_dt_read_ddr(rd_dt_data, 4, wr_dt_addr);
                    athr_dt_data_check(& check_result, rd_dt_data,
                        ddr_dt_pattern,
                        athr_ddr_init_val.lane_number,
                        athr_ddr_init_val.burst_length,
                        ddr_dly_type_loop);

                    /* record the delay code according to the check result  */
                    if ((1 == check_result_record) && (0 == check_result)) {
                        break;
                    }

		            if (1 == check_result) {
                        ddr_dly_high_thr = ddr_dly_loop;
                    }

                    if ((0 == check_result_record) && (1 == check_result)) {
                        ddr_dly_low_thr = ddr_dly_loop;
                        continue;
                    }
                }

                /* record the rising/falling edge delay
                    code of current dqs_in_gate_phase */
                switch(ddr_dly_type_loop) {
                    case 0:
                        rdly_val    = ((ddr_dly_high_thr + ddr_dly_low_thr) / 2);
			            rdly_range  = (ddr_dly_high_thr - ddr_dly_low_thr);
                        DDR_DEBUG("DT: ddr_rdly_low_thr = 0x%x ddr_rdly_high_thr = 0x%x rdly_range = 0x%x\n",
                            ddr_dly_low_thr, ddr_dly_high_thr, rdly_range);
                         break;
                    case 1:
			            fdly_val    = ((ddr_dly_high_thr + ddr_dly_low_thr) / 2);
			            fdly_range  = (ddr_dly_high_thr - ddr_dly_low_thr);
                        DDR_DEBUG("DT: ddr_fdly_low_thr = 0x%x, ddr_fdly_high_thr = 0x%x fdly_range = 0x%x\n",
                            ddr_dly_low_thr, ddr_dly_high_thr, fdly_range);
                        break;
			        default:
			            break;
                }

		        if (2 * rdly_val > 0x3f) {
                    edly_val = 0x3f;
		        } else {
		            edly_val = 2 * rdly_val;
		        }

            }

	        if ((rdly_range >= rdly_range_record) && (fdly_range >= fdly_range_record)) {
                rdly_range_record = rdly_range;
                fdly_range_record = fdly_range;
                rdly_val_record = rdly_val;
	            fdly_val_record = fdly_val;
		        edly_val_record = edly_val;
		        dqs_in_gate_val_record = ddr_dt_qds_loop;
                DDR_DEBUG("\nDT: rdly_range = 0x%x rdly_range_record = 0x%x"
                    " fdly_range = 0x%x fdly_range_record = 0x%x \n",
                    rdly_range,rdly_range_record, fdly_range, fdly_range_record);
	        }

			if(ddr_dt_qds_loop == 3) {
				ddr_dt_qds_loop = 0;

			} else {
				if(ddr_dt_qds_loop++ == 2) {
					break;
				}
			}
        }

        /* write train-out code into register */
        switch(ddr_lane_loop) {
            case 0:
		        athr_dt_phy_ctrl1.content.DQS_IN_RDLY_CODE_OFF0 = rdly_val_record;
                athr_dt_phy_ctrl1.content.DQS_IN_FDLY_CODE_OFF0 = fdly_val_record;
		        athr_dt_phy_ctrl0.content.DQS_IN_EDLY_CODE_OFF0 = edly_val_record;
		        athr_dt_dqs_gate_phase.content.DQS_IN_GATE_PHASE_LANE0 = dqs_in_gate_val_record;
                printf("\nDT: lane0 PHASE = 0x%x RDLY = 0x%x FDLY = 0x%x EDLY = 0x%x \n",
                    dqs_in_gate_val_record, rdly_val_record, fdly_val_record, edly_val_record);
                break;
            case 1:
                athr_dt_phy_ctrl1.content.DQS_IN_RDLY_CODE_OFF1 = rdly_val_record;
                athr_dt_phy_ctrl1.content.DQS_IN_FDLY_CODE_OFF1 = fdly_val_record;
		        athr_dt_phy_ctrl0.content.DQS_IN_EDLY_CODE_OFF1 = edly_val_record;
		        athr_dt_dqs_gate_phase.content.DQS_IN_GATE_PHASE_LANE1 = dqs_in_gate_val_record;
                printf("\nDT: lane1 PHASE = 0x%x RDLY = 0x%x FDLY = 0x%x EDLY = 0x%x \n",
                    dqs_in_gate_val_record, rdly_val_record, fdly_val_record, edly_val_record);
                break;
            default:
                break;
        }

        music_reg_wr_nf(DDR_PHY_RD_CTRL1_REG, athr_dt_phy_ctrl1.val);
        music_reg_wr_nf(DDR_PHY_RD_CTRL0_REG, athr_dt_phy_ctrl0.val);
        music_reg_wr_nf(DQS_GATE_PHASE_REG, athr_dt_dqs_gate_phase.val);
    }

    /* iserdes flush issue */
    athr_dt_phy_misc.val = music_reg_rd(DDR_PHY_MISC_REG);
    athr_dt_phy_misc.content.ISERDES_FLUSH = 1;
    athr_dt_phy_misc.content.CODE_SET_MANU_EN = 1;
    music_reg_wr_nf(DDR_PHY_MISC_REG, athr_dt_phy_misc.val);

    DDR_DEBUG("DT: write addr 0x%x val 0x%x\r\n", DDR_PHY_MISC_REG, athr_dt_phy_misc.val);

    /* wait iserdes flush done */
    ddr_timer_delay(DDR_I_SERDES_FLUSH_TIME);

    /* set ddr_control burst length 4 to 8 the clear status
        for hardware bug if use burst length= 8 training    */
    if (DDR2_BURSTLENGTH_8 == athr_ddr_init_val.burst_length) {
        athr_dt_ddr_config1.val = music_reg_rd(DDR_CONFIG1_REG);
	    athr_dt_ddr_config1.content.BURST_LENGTH = 4;
	    music_reg_wr_nf(DDR_CONFIG1_REG, athr_dt_ddr_config1.val);
	    athr_dt_ddr_config1.content.BURST_LENGTH = 8;
	    music_reg_wr_nf(DDR_CONFIG1_REG, athr_dt_ddr_config1.val);
    }

    /* set training done */
    memset(&athr_dt_ddr_cfg, 0, sizeof(athr_dt_ddr_cfg));
    //athr_dt_ddr_cfg.val = music_reg_rd(DDR_CFG_REG);
    athr_dt_ddr_cfg.content.DT_DONE = 1;
    music_reg_wr_nf(DDR_CFG_REG, athr_dt_ddr_cfg.val);
    DDR_DEBUG("DT: write addr 0x%x val 0x%x\r\n", DDR_CFG_REG, athr_dt_ddr_cfg.val);


    /* set REOD_BP = 0 for normal ddr access */
    athr_dt_misc.val = music_reg_rd(DDR_CTRL_MISC_REG);
    athr_dt_misc.content.REOD_BP = 0;
    music_reg_wr_nf(DDR_CTRL_MISC_REG, athr_dt_misc.val);
    DDR_DEBUG("DT: write addr 0x%x val 0x%x\r\n", DDR_CTRL_MISC_REG, athr_dt_misc.val);

    DDR_DEBUG("finish Data Train\r\n");

    return 0;
}


int32_t athr_ddr_config_dump(struct athr_ddr_init_config dump_config)
{
    DDR_DEBUG("Dump ddr config \n");
    DDR_DEBUG("\tconfig->burst_length = 0x%x\n",
        dump_config.burst_length);
    DDR_DEBUG("\tconfig->burst_type = 0x%x\n",
        dump_config.burst_type);
    DDR_DEBUG("\tconfig->CAS_latency = 0x%x\n",
        dump_config.CAS_latency);
    DDR_DEBUG("\tconfig->write_recovery = 0x%x\n",
        dump_config.write_recovery);
    DDR_DEBUG("\tconfig->density = 0x%x\n",
        dump_config.density);
    DDR_DEBUG("\tconfig->lane_number = 0x%x\n",
        dump_config.lane_number);
    DDR_DEBUG("\tconfig->ddr_mode = 0x%x\n",
        dump_config.ddr_mode);
    DDR_DEBUG("\tconfig->addr_map_mode = 0x%x\n",
        dump_config.addr_map_mode);
    DDR_DEBUG("\tconfig->clock = 0x%x\n",
        dump_config.clock);
    return 0;
}
