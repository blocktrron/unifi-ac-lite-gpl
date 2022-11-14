#include "switch_48p.h"
#include "switch_48p_api.h"

void switch_master_trunk_stk_set(uint32_t master_dev_id,
                                          uint32_t slave_dev_id,
                                          uint32_t stk_id,
                                          uint32_t *port_array,
                                          uint32_t nr_port)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;
    uint32_t i;

    /*stack trunk member*/
    i = 0;
    reg_val = 0;
    reg_addr = STK_TRUNK_MEM_0+stk_id*STK_TRUNK_MEM_0_INC;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM0_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM1_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM2_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM3_OFFSET);
    i++;
    music_switch_wr(reg_addr, reg_val);
    reg_val = 0;
    reg_addr = STK_TRUNK_MEM_1+stk_id*STK_TRUNK_MEM_1_INC;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM4_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM5_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM6_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM7_OFFSET);
    i++;
    music_switch_wr(reg_addr, reg_val);



	/*uc filter*/
	reg_val = 0;
	reg_addr = MUSIC_STACKING_UNICAST_FWD+master_dev_id*MUSIC_STACKING_UNICAST_FWD_INC;
	music_switch_wr(reg_addr, reg_val);

	reg_val = 0;
	reg_addr = MUSIC_STACKING_UNICAST_FWD+slave_dev_id*MUSIC_STACKING_UNICAST_FWD_INC;
	music_switch_wr(reg_addr, reg_val);

	/*mc filter*/
	reg_val = 0x3fffffff;
	reg_addr = MUSIC_STACKING_MUL_FILT+master_dev_id*MUSIC_STACKING_MUL_FILT_INC;
	music_switch_wr(reg_addr, reg_val);

	reg_val = 0x3fffffff;
	reg_addr = MUSIC_STACKING_MUL_FILT+slave_dev_id*MUSIC_STACKING_MUL_FILT_INC;
	music_switch_wr(reg_addr, reg_val);

    /*port cfg*/
    for(i=0;i<nr_port;i++)
    {

        /*uc filter*/
        reg_val = 0;
        reg_addr = MUSIC_STACKING_UNICAST_FWD+slave_dev_id*MUSIC_STACKING_UNICAST_FWD_INC;
        reg_val = music_switch_rd(reg_addr);
        reg_val |= (1<<port_array[i]);
        music_switch_wr(reg_addr, reg_val);

        /*mc filter*/
        reg_val = 0;
        reg_addr = MUSIC_STACKING_MUL_FILT+slave_dev_id*MUSIC_STACKING_MUL_FILT_INC;
        reg_val = music_switch_rd(reg_addr);
        reg_val &= ~(1<<port_array[i]);
        music_switch_wr(reg_addr, reg_val);



        /*dna header*/
        reg_val = 0;
        reg_addr = EG_PORT_TAG_CTRL+port_array[i]*EG_PORT_TAG_CTRL_INC;
        reg_val = music_switch_rd(reg_addr);
        reg_val |= (PORT_AHP_TAG_EN|PORT_EG_DATA_TAG_EN|PORT_EG_MGMT_TAG_EN);
        music_switch_wr(reg_addr, reg_val);

        /*jumbo*/
        reg_val = 0;
        reg_addr = PORT_MISC_CFG+port_array[i]*PORT_MISC_CFG_INC;
        reg_val = music_switch_rd(reg_addr);
        reg_val |= PORT_JUMBO_EN;
        music_switch_wr(reg_addr, reg_val);

        /*port status cfg*/
        reg_val = 0;
        reg_addr = PORT_STATUS_CONFIG+port_array[i]*PORT_STATUS_CONFIG_INC;
        reg_val = music_switch_rd(reg_addr);
		reg_val &= ~AUTO_FLOW_CTRL_EN;
		reg_val &= ~AUTO_LINK_EN;
        reg_val |= (RXMAC_EN|TXMAC_EN);

        music_switch_wr(reg_addr, reg_val);
    }

    return;
}

/*xg_index 0 means port25*/
/*xg_index 1 means port26*/
void switch_master_xg_port_init(uint32_t xg_index)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_val = 0;
    reg_addr = SD_CLK_SEL;
    reg_val = 0x000000ff;
    music_switch_wr(reg_addr, reg_val);

    reg_val = 0;
    reg_addr = XGMAC_PORT_CONTROL+xg_index*XGMAC_PORT_CONTROL_INC;
    reg_val = music_switch_rd(reg_addr);
    reg_val |= (PROMIS_EN|RX_ENA|TX_ENA);
    music_switch_wr(reg_addr, reg_val);

    if(0 == xg_index)
    {
        /*port 25*/
        reg_val = 0;
        reg_addr = GLOBAL_CTRL_1;
        reg_val = music_switch_rd(reg_addr);
        reg_val |= MAC25XG_4P3G_EN;
        reg_val &= ~MAC25XG_3P125G_EN;
        music_switch_wr(reg_addr, reg_val);

        reg_val = 0;
        reg_addr = XAUI_SERDES9_CTRL0;
        reg_val = music_switch_rd(reg_addr);
        reg_val |= (0xf<<XAUI_SERDES9_EN_SD_OFFSET);
        music_switch_wr(reg_addr, reg_val);

        reg_val = 0;
        reg_addr = XAUI_SERDES9_CTRL1;
        reg_val = music_switch_rd(reg_addr);
        reg_val |= (0xf<<XAUI_SERDES9_EN_RX_OFFSET|0xf<<XAUI_SERDES9_EN_TX_OFFSET);
		reg_val |= (XAUI_SERDES9_BIT19|XAUI_SERDES9_BIT20);
        music_switch_wr(reg_addr, reg_val);
    }
    else
    {
        /*port 26*/
        reg_val = 0;
        reg_addr = GLOBAL_CTRL_1;
        reg_val = music_switch_rd(reg_addr);
        reg_val |= MAC26XG_4P3G_EN;
        reg_val &= ~(MAC29SG_1P25G_EN|MAC29SG_3P125G_EN
                    |MAC28SG_1P25G_EN|MAC28SG_3P125G_EN
                    |MAC27SG_1P25G_EN|MAC27SG_3P125G_EN
                    |MAC26SG_1P25G_EN|MAC26SG_3P125G_EN
                    |MAC26XG_3P125G_EN);
        music_switch_wr(reg_addr, reg_val);

        reg_val = 0;
        reg_addr = XAUI_SERDES13_CTRL0;
        reg_val = music_switch_rd(reg_addr);
        reg_val |= (0xf << XAUI_SERDES13_EN_SD_OFFSET);
        music_switch_wr(reg_addr, reg_val);

        reg_val = 0;
        reg_addr = XAUI_SERDES13_CTRL1;
        reg_val = music_switch_rd(reg_addr);
        reg_val |= (0xf<<XAUI_SERDES13_EN_RX_OFFSET|0xf<<XAUI_SERDES13_EN_TX_OFFSET);
		reg_val |= (XAUI_SERDES13_BIT19|XAUI_SERDES13_BIT20);
        music_switch_wr(reg_addr, reg_val);
    }

    return;
}

void switch_master_global_flowcontrol_threshold_set(uint32_t xon_threshold, uint32_t xoff_threshold)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_val = 0;
    reg_addr = GOL_FLOW_THD;
    reg_val = music_switch_rd(reg_addr);
    SWITCH_REG_FIELD_SET(reg_val, xon_threshold, GOL_FLOW_THD_GOL_XON_THRES_OFFSET, GOL_FLOW_THD_GOL_XON_THRES_LEN);
    SWITCH_REG_FIELD_SET(reg_val, xoff_threshold, GOL_FLOW_THD_GOL_XOFF_THRES_OFFSET, GOL_FLOW_THD_GOL_XOFF_THRES_LEN);
    music_switch_wr(reg_addr, reg_val);

    return;
}

void switch_master_port_flowcontrol_threshold_set(uint32_t port_id, uint32_t xon_threshold, uint32_t xoff_threshold)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_val = 0;
    reg_addr = PORT_FLOW_THD+port_id*PORT_FLOW_THD_INC;
    reg_val = music_switch_rd(reg_addr);
    SWITCH_REG_FIELD_SET(reg_val, xon_threshold, PORT_FLOW_THD_PORT_XON_THRES_OFFSET, PORT_FLOW_THD_PORT_XON_THRES_LEN);
    SWITCH_REG_FIELD_SET(reg_val, xoff_threshold, PORT_FLOW_THD_PORT_XOFF_THRES_OFFSET, PORT_FLOW_THD_PORT_XOFF_THRES_LEN);
    music_switch_wr(reg_addr, reg_val);

    return;
}

void switch_master_port_wred_en_set(uint32_t port_id, uint32_t enable)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_val = 0;
    reg_addr = PORT_DYN_ALLOC_0 + port_id*PORT_DYN_ALLOC_0_INC;
    reg_val = music_switch_rd(reg_addr);

	if(0 == enable)
	{
	    reg_val &= ~PORT_DYN_ALLOC_0_PORT_WRED_EN;
	}
	else
	{
	    reg_val |= PORT_DYN_ALLOC_0_PORT_WRED_EN;
	}

    music_switch_wr(reg_addr, reg_val);
}

/*
value1 for q0/q1
value2 for q2/q3

*/
void switch_master_port_holb1_set(uint32_t port_id, uint32_t value1)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_addr = PORT_DYN_ALLOC_1 + port_id*PORT_DYN_ALLOC_1_INC;
    reg_val = value1;
    music_switch_wr(reg_addr, reg_val);
}
void switch_master_port_holb2_set(uint32_t port_id, uint32_t value2)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

	reg_addr = PORT_DYN_ALLOC_2 + port_id*PORT_DYN_ALLOC_2_INC;  ;
	reg_val = value2;
	music_switch_wr(reg_addr, reg_val);
}

void switch_master_port_holb3_set(uint32_t port_id, uint32_t value3)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

	reg_addr = PORT_PRE_ALLOC_0 + port_id*PORT_PRE_ALLOC_0_INC; ;
	reg_val = value3;
	music_switch_wr(reg_addr, reg_val);
}

void switch_slave_port_wred_en_set(uint32_t port_id, uint32_t enable)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_val = 0;
    reg_addr = PORT_DYN_ALLOC_0 + port_id*PORT_DYN_ALLOC_0_INC;
    reg_val = mdio_switch_reg_rd(reg_addr);

	if(0 == enable)
	{
	    reg_val &= ~PORT_DYN_ALLOC_0_PORT_WRED_EN;
	}
	else
	{
	    reg_val |= PORT_DYN_ALLOC_0_PORT_WRED_EN;
	}

    mdio_switch_reg_wr(reg_addr, reg_val);
}

/*
value1 for q0/q1
value2 for q2/q3

*/
void switch_slave_port_holb1_set(uint32_t port_id, uint32_t value1)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_addr = PORT_DYN_ALLOC_1 + port_id*PORT_DYN_ALLOC_1_INC; ;
    reg_val = value1;
    mdio_switch_reg_wr(reg_addr, reg_val);

}
void switch_slave_port_holb2_set(uint32_t port_id, uint32_t value2)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

	reg_addr = PORT_DYN_ALLOC_2 + port_id*PORT_DYN_ALLOC_2_INC; ;
	reg_val = value2;
	mdio_switch_reg_wr(reg_addr, reg_val);
}

void switch_slave_port_holb3_set(uint32_t port_id, uint32_t value3)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

	reg_addr = PORT_PRE_ALLOC_0 + port_id*PORT_PRE_ALLOC_0_INC; ;
	reg_val = value3;
	mdio_switch_reg_wr(reg_addr, reg_val);
}


void switch_master_stkport_fc_thd_set(uint32_t xon_thd, uint32_t xoff_thd)
{
	switch_master_port_flowcontrol_threshold_set(25, xon_thd, xoff_thd);
	switch_master_port_flowcontrol_threshold_set(26, xon_thd, xoff_thd);
}

void switch_slave_stkport_fc_thd_set(uint32_t xon_thd, uint32_t xoff_thd)
{
	switch_slave_port_flowcontrol_threshold_set(25, xon_thd, xoff_thd);
	switch_slave_port_flowcontrol_threshold_set(26, xon_thd, xoff_thd);
}


void switch_master_24ports_fc_thd_set(uint32_t xon_thd, uint32_t xoff_thd)
{
	uint32_t port = 0;

    for(port = SWITCH_PORT_CFG_START; port <= 24; port ++) {
		switch_master_port_flowcontrol_threshold_set(port, xon_thd, xoff_thd);
    }
}

void switch_slave_24ports_fc_thd_set(uint32_t xon_thd, uint32_t xoff_thd)
{
	uint32_t port = 0;

    for(port = SWITCH_PORT_CFG_START; port <= 24; port ++) {
		switch_slave_port_flowcontrol_threshold_set(port, xon_thd, xoff_thd);
    }
}

void switch_master_stk_queue_holb1_set(uint32_t value1)
{
	switch_master_port_holb1_set(25, value1);
	switch_master_port_holb1_set(26, value1);
}

void switch_slave_stk_queue_holb1_set(uint32_t value1)
{
	switch_slave_port_holb1_set(25, value1);
	switch_slave_port_holb1_set(26, value1);
}

void switch_master_stk_queue_holb2_set(uint32_t value2)
{
	switch_master_port_holb2_set(25, value2);
	switch_master_port_holb2_set(26, value2);
}

void switch_slave_stk_queue_holb2_set(uint32_t value2)
{
	switch_slave_port_holb2_set(25, value2);
	switch_slave_port_holb2_set(26, value2);
}


void switch_master_24ports_queue_holb1_set(uint32_t value1)
{
	uint32_t port = 0;

    for(port = SWITCH_PORT_CFG_START; port <= 24; port ++) {
		switch_master_port_holb1_set(port, value1);
    }
}

void switch_slave_24ports_queue_holb1_set(uint32_t value1)
{
	uint32_t port = 0;

    for(port = SWITCH_PORT_CFG_START; port <= 24; port ++) {
		switch_slave_port_holb1_set(port, value1);
    }
}

void switch_master_24ports_queue_holb2_set(uint32_t value2)
{
	uint32_t port = 0;

    for(port = SWITCH_PORT_CFG_START; port <= 24; port ++) {
		switch_master_port_holb2_set(port, value2);
    }
}

void switch_master_24ports_queue_holb3_set(uint32_t value3)
{
	uint32_t port = 0;

    for(port = SWITCH_PORT_CFG_START; port <= 24; port ++) {
		switch_master_port_holb3_set(port, value3);
    }
}

void switch_slave_24ports_queue_holb2_set(uint32_t value2)
{
	uint32_t port = 0;

    for(port = SWITCH_PORT_CFG_START; port <= 24; port ++) {
	switch_slave_port_holb2_set(port, value2);
    }
}

void switch_slave_24ports_queue_holb3_set(uint32_t value3)
{
	uint32_t port = 0;

    for(port = SWITCH_PORT_CFG_START; port <= 24; port ++) {
	switch_slave_port_holb3_set(port, value3);
    }
}

void switch_master_stk_queue_holb3_set(uint32_t value3)
{
	switch_master_port_holb3_set(25, value3);
	switch_master_port_holb3_set(26, value3);
}

void switch_slave_stk_queue_holb3_set(uint32_t value3)
{
	switch_slave_port_holb3_set(25, value3);
	switch_slave_port_holb3_set(26, value3);
}

void switch_master_stk_wred_set(uint32_t enable)
{
	switch_master_port_wred_en_set(25, enable);
	switch_master_port_wred_en_set(26, enable);
}

void switch_slave_stk_wred_set(uint32_t enable)
{
	switch_slave_port_wred_en_set(25, enable);
	switch_slave_port_wred_en_set(26, enable);
}

void switch_master_24ports_wred_set(uint32_t enable)
{
	uint32_t port = 0;

    for(port = SWITCH_PORT_CFG_START; port <= 24; port ++) {
		switch_master_port_wred_en_set(port, enable);
    }
}

void switch_slave_24ports_wred_set(uint32_t enable)
{
	uint32_t port = 0;

    for(port = SWITCH_PORT_CFG_START; port <= 24; port ++) {
		switch_slave_port_wred_en_set(port, enable);
    }
}

void switch_master_uplink_wred_set(uint32_t enable)
{
	switch_master_port_wred_en_set(SWITCH_26PORT_UPLINK_PORT_ID, enable);
	switch_master_port_wred_en_set(SWITCH_27PORT_UPLINK_PORT_ID, enable);
}

void switch_master_uplink_fc_thd_set(uint32_t xon_thd, uint32_t xoff_thd)
{
	switch_master_port_flowcontrol_threshold_set(SWITCH_26PORT_UPLINK_PORT_ID, xon_thd, xoff_thd);
	switch_master_port_flowcontrol_threshold_set(SWITCH_27PORT_UPLINK_PORT_ID, xon_thd, xoff_thd);
}

void switch_master_uplink_queue_holb1_set(uint32_t value1)
{
	switch_master_port_holb1_set(SWITCH_26PORT_UPLINK_PORT_ID, value1);
	switch_master_port_holb1_set(SWITCH_27PORT_UPLINK_PORT_ID, value1);
}

void switch_master_uplink_queue_holb2_set(uint32_t value2)
{
	switch_master_port_holb2_set(SWITCH_26PORT_UPLINK_PORT_ID, value2);
	switch_master_port_holb2_set(SWITCH_27PORT_UPLINK_PORT_ID, value2);
}

void switch_master_uplink_queue_holb3_set(uint32_t value3)
{
	switch_master_port_holb3_set(SWITCH_26PORT_UPLINK_PORT_ID, value3);
	switch_master_port_holb3_set(SWITCH_27PORT_UPLINK_PORT_ID, value3);
}

/*
usually port_id is 25/26
enable 0 for disable;otherwise for enable
*/
void switch_master_port_flowcontrol_mode_set(uint32_t port_id, uint32_t enable)
{
        uint32_t reg_val = 0;
        uint32_t reg_addr = 0;

        reg_val = 0;
        reg_addr = PORT_STATUS_CONFIG+port_id*PORT_STATUS_CONFIG_INC;
        reg_val = music_switch_rd(reg_addr);

        if(0 == enable)
        {
			reg_val &= ~TX_FLOW_EN;
			reg_val &= ~RX_FLOW_EN;
        }
        else
        {
			reg_val |= TX_FLOW_EN;
			reg_val |= RX_FLOW_EN;
        }

        music_switch_wr(reg_addr, reg_val);

        return;
}

void switch_master_fwd_dev_id_set(uint32_t dev_id)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_val = 0;
    reg_addr = DEV_ID;
    reg_val = music_switch_rd(reg_addr);
    SWITCH_REG_FIELD_SET(reg_val, dev_id, DEV_ID_DEV_ID_OFFSET, DEV_ID_DEV_ID_LEN);
    music_switch_wr(reg_addr, reg_val);

    return;
}

void switch_master_stk_flowcontrol_mode_set(uint32_t enable)
{
	switch_master_port_flowcontrol_mode_set(25, enable);
	switch_master_port_flowcontrol_mode_set(26, enable);
}

//void swith_master_init(void)
void master_switch_stack_init(void)
{
    uint32_t port_array[2];
	uint32_t nr_port;

    switch_master_fwd_dev_id_set(0);

	port_array[0]=25;
	port_array[1]=26;
	nr_port = 2;

    switch_master_trunk_stk_set(0,1,0, port_array, nr_port);
	switch_master_xg_port_init(0);
	switch_master_xg_port_init(1);

	/*disable by default*/
	switch_master_stk_flowcontrol_mode_set(0);

}

/*slave cfg use mdio_switch_reg_rd/mdio_switch_reg_wr*/
void switch_slave_trunk_stk_set(uint32_t master_dev_id,
                                        uint32_t slave_dev_id,
                                        uint32_t stk_id,
                                        uint32_t *port_array,
                                        uint32_t nr_port)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;
    uint32_t i;

    /*stack trunk member*/
    i = 0;
    reg_val = 0;
    reg_addr = STK_TRUNK_MEM_0+stk_id*STK_TRUNK_MEM_0_INC;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM0_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM1_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM2_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM3_OFFSET);
    i++;
    mdio_switch_reg_wr(reg_addr, reg_val);
    reg_val = 0;
    reg_addr = STK_TRUNK_MEM_1+stk_id*STK_TRUNK_MEM_1_INC;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM4_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM5_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM6_OFFSET);
    i++;
    reg_val |= (port_array[i%nr_port]<<STK_TRUNK_MEM7_OFFSET);
    i++;
    mdio_switch_reg_wr(reg_addr, reg_val);

	/*uc filter*/
	reg_val = 0;
	reg_addr = MUSIC_STACKING_UNICAST_FWD+master_dev_id*MUSIC_STACKING_UNICAST_FWD_INC;
	mdio_switch_reg_wr(reg_addr, reg_val);

	reg_val = 0;
	reg_addr = MUSIC_STACKING_UNICAST_FWD+slave_dev_id*MUSIC_STACKING_UNICAST_FWD_INC;
	mdio_switch_reg_wr(reg_addr, reg_val);

	/*mc filter*/
	reg_val = 0x3fffffff;
	reg_addr = MUSIC_STACKING_MUL_FILT+master_dev_id*MUSIC_STACKING_MUL_FILT_INC;
	mdio_switch_reg_wr(reg_addr, reg_val);

	reg_val = 0x3fffffff;
	reg_addr = MUSIC_STACKING_MUL_FILT+slave_dev_id*MUSIC_STACKING_MUL_FILT_INC;
	mdio_switch_reg_wr(reg_addr, reg_val);


    /*port cfg*/
    for(i=0;i<nr_port;i++)
    {

        /*uc filter*/
        reg_val = 0;
        reg_addr = MUSIC_STACKING_UNICAST_FWD+master_dev_id*MUSIC_STACKING_UNICAST_FWD_INC;
        reg_val = mdio_switch_reg_rd(reg_addr);
        reg_val |= (1<<port_array[i]);
        mdio_switch_reg_wr(reg_addr, reg_val);

        /*mc filter*/
        reg_val = 0;
        reg_addr = MUSIC_STACKING_MUL_FILT+master_dev_id*MUSIC_STACKING_MUL_FILT_INC;
        reg_val = mdio_switch_reg_rd(reg_addr);
        reg_val &= ~(1<<port_array[i]);
        mdio_switch_reg_wr(reg_addr, reg_val);




        /*dna header*/
        reg_val = 0;
        reg_addr = EG_PORT_TAG_CTRL+port_array[i]*EG_PORT_TAG_CTRL_INC;
        reg_val = mdio_switch_reg_rd(reg_addr);
        reg_val |= (PORT_AHP_TAG_EN|PORT_EG_DATA_TAG_EN|PORT_EG_MGMT_TAG_EN);
        mdio_switch_reg_wr(reg_addr, reg_val);

        /*jumbo*/
        reg_val = 0;
        reg_addr = PORT_MISC_CFG+port_array[i]*PORT_MISC_CFG_INC;
        reg_val = mdio_switch_reg_rd(reg_addr);
        reg_val |= PORT_JUMBO_EN;
        mdio_switch_reg_wr(reg_addr, reg_val);

        /*port status cfg*/
        reg_val = 0;
        reg_addr = PORT_STATUS_CONFIG+port_array[i]*PORT_STATUS_CONFIG_INC;
        reg_val = mdio_switch_reg_rd(reg_addr);
		reg_val &= ~AUTO_FLOW_CTRL_EN;
		reg_val &= ~AUTO_LINK_EN;
        reg_val |= (RXMAC_EN|TXMAC_EN);
        mdio_switch_reg_wr(reg_addr, reg_val);
    }

    return;

}

/*xg_index 0 means port25*/
/*xg_index 1 means port26*/
void switch_slave_xg_port_init(uint32_t xg_index)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_val = 0;
    reg_addr = SD_CLK_SEL;
    reg_val = 0x000000ff;
    mdio_switch_reg_wr(reg_addr, reg_val);

    reg_val = 0;
    reg_addr = XGMAC_PORT_CONTROL+xg_index*XGMAC_PORT_CONTROL_INC;
    reg_val = mdio_switch_reg_rd(reg_addr);
    reg_val |= (PROMIS_EN|RX_ENA|TX_ENA);
    mdio_switch_reg_wr(reg_addr, reg_val);

    if(0 == xg_index)
    {
        /*port 25*/
        reg_val = 0;
        reg_addr = GLOBAL_CTRL_1;
        reg_val = mdio_switch_reg_rd(reg_addr);
        reg_val |= MAC25XG_4P3G_EN;
        reg_val &= ~MAC25XG_3P125G_EN;
        mdio_switch_reg_wr(reg_addr, reg_val);

        reg_val = 0;
        reg_addr = XAUI_SERDES9_CTRL0;
        reg_val = mdio_switch_reg_rd(reg_addr);
        reg_val |= (0xf<<XAUI_SERDES9_EN_SD_OFFSET);
        mdio_switch_reg_wr(reg_addr, reg_val);

        reg_val = 0;
        reg_addr = XAUI_SERDES9_CTRL1;
        reg_val = mdio_switch_reg_rd(reg_addr);
        reg_val |= (0xf<<XAUI_SERDES9_EN_RX_OFFSET|0xf<<XAUI_SERDES9_EN_TX_OFFSET);
		reg_val |= (XAUI_SERDES9_BIT19|XAUI_SERDES9_BIT20);
        mdio_switch_reg_wr(reg_addr, reg_val);
    }
    else
    {
        /*port 26*/
        reg_val = 0;
        reg_addr = GLOBAL_CTRL_1;
        reg_val = mdio_switch_reg_rd(reg_addr);
        reg_val |= MAC26XG_4P3G_EN;
        reg_val &= ~(MAC29SG_1P25G_EN|MAC29SG_3P125G_EN
                    |MAC28SG_1P25G_EN|MAC28SG_3P125G_EN
                    |MAC27SG_1P25G_EN|MAC27SG_3P125G_EN
                    |MAC26SG_1P25G_EN|MAC26SG_3P125G_EN
                    |MAC26XG_3P125G_EN);
        mdio_switch_reg_wr(reg_addr, reg_val);

        reg_val = 0;
        reg_addr = XAUI_SERDES13_CTRL0;
        reg_val = mdio_switch_reg_rd(reg_addr);
        reg_val |= (0xf << XAUI_SERDES13_EN_SD_OFFSET);
        mdio_switch_reg_wr(reg_addr, reg_val);

        reg_val = 0;
        reg_addr = XAUI_SERDES13_CTRL1;
        reg_val = mdio_switch_reg_rd(reg_addr);
        reg_val |= (0xf<<XAUI_SERDES13_EN_RX_OFFSET|0xf<<XAUI_SERDES13_EN_TX_OFFSET);
		reg_val |= (XAUI_SERDES13_BIT19|XAUI_SERDES13_BIT20);
        mdio_switch_reg_wr(reg_addr, reg_val);
    }

    return;
}

void switch_slave_global_flowcontrol_threshold_set(uint32_t xon_threshold, uint32_t xoff_threshold)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_val = 0;
    reg_addr = GOL_FLOW_THD;
    reg_val = mdio_switch_reg_rd(reg_addr);
    SWITCH_REG_FIELD_SET(reg_val, xon_threshold, GOL_FLOW_THD_GOL_XON_THRES_OFFSET, GOL_FLOW_THD_GOL_XON_THRES_LEN);
    SWITCH_REG_FIELD_SET(reg_val, xoff_threshold, GOL_FLOW_THD_GOL_XOFF_THRES_OFFSET, GOL_FLOW_THD_GOL_XOFF_THRES_LEN);
    mdio_switch_reg_wr(reg_addr, reg_val);

    return;
}

void switch_slave_port_flowcontrol_threshold_set(uint32_t port_id, uint32_t xon_threshold, uint32_t xoff_threshold)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_val = 0;
    reg_addr = PORT_FLOW_THD+port_id*PORT_FLOW_THD_INC;
    reg_val = mdio_switch_reg_rd(reg_addr);
    SWITCH_REG_FIELD_SET(reg_val, xon_threshold, PORT_FLOW_THD_PORT_XON_THRES_OFFSET, PORT_FLOW_THD_PORT_XON_THRES_LEN);
    SWITCH_REG_FIELD_SET(reg_val, xoff_threshold, PORT_FLOW_THD_PORT_XOFF_THRES_OFFSET, PORT_FLOW_THD_PORT_XOFF_THRES_LEN);
    mdio_switch_reg_wr(reg_addr, reg_val);

    return;
}

/*
usually port_id is 25/26
enable 0 for disable;otherwise for enable
*/
void switch_slave_port_flowcontrol_mode_set(uint32_t port_id, uint32_t enable)
{
        uint32_t reg_val = 0;
        uint32_t reg_addr = 0;

        reg_val = 0;
        reg_addr = PORT_STATUS_CONFIG+port_id*PORT_STATUS_CONFIG_INC;
        reg_val = mdio_switch_reg_rd(reg_addr);

        if(0 == enable)
        {
			reg_val &= ~TX_FLOW_EN;
			reg_val &= ~RX_FLOW_EN;
        }
        else
        {
			reg_val |= TX_FLOW_EN;
			reg_val |= RX_FLOW_EN;
        }

        mdio_switch_reg_wr(reg_addr, reg_val);

        return;
}

void switch_slave_fwd_dev_id_set(uint32_t dev_id)
{
    uint32_t reg_val = 0;
    uint32_t reg_addr = 0;

    reg_val = 0;
    reg_addr = DEV_ID;
    reg_val = mdio_switch_reg_rd(reg_addr);
    SWITCH_REG_FIELD_SET(reg_val, dev_id, DEV_ID_DEV_ID_OFFSET, DEV_ID_DEV_ID_LEN);
    mdio_switch_reg_wr(reg_addr, reg_val);

    return;
}

void switch_slave_stk_flowcontrol_mode_set(uint32_t enable)
{
	switch_slave_port_flowcontrol_mode_set(25, enable);
	switch_slave_port_flowcontrol_mode_set(26, enable);
}

void slave_switch_stack_init(void)
{
    uint32_t port_array[2];
	uint32_t nr_port;

    switch_slave_fwd_dev_id_set(1);

	port_array[0]=25;
	port_array[1]=26;
	nr_port = 2;

    switch_slave_trunk_stk_set(0,1,0, port_array, nr_port);
	switch_slave_xg_port_init(0);
	switch_slave_xg_port_init(1);

	/*disable by default*/
	switch_slave_stk_flowcontrol_mode_set(0);
}
