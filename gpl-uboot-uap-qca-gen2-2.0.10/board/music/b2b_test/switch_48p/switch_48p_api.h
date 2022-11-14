#ifndef _SWITCH_48P_API_H__
#define _SWITCH_48P_API_H__

void switch_stack_init(void);
void master_switch_stack_init(void);
void slave_switch_stack_init(void);

void switch_master_trunk_stk_set(uint32_t master_dev_id,
                                          uint32_t slave_dev_id,
                                          uint32_t stk_id,
                                          uint32_t *port_array,
                                          uint32_t nr_port);

/*xg_index 0 means port25*/
/*xg_index 1 means port26*/
void switch_master_xg_port_init(uint32_t xg_index);
void switch_master_global_flowcontrol_threshold_set(uint32_t xon_threshold, uint32_t xoff_threshold);
void switch_master_port_flowcontrol_threshold_set(uint32_t port_id, uint32_t xon_threshold, uint32_t xoff_threshold);
void switch_master_port_flowcontrol_mode_set(uint32_t port_id, uint32_t enable);
void switch_master_fwd_dev_id_set(uint32_t dev_id);

void swith_master_init(void);

/*slave cfg use soc_mdio_rd/soc_mdio_wr*/
void switch_slave_trunk_stk_set(uint32_t master_dev_id,
                                        uint32_t slave_dev_id,
                                        uint32_t stk_id,
                                        uint32_t *port_array,
                                        uint32_t nr_port);

void switch_slave_xg_port_init(uint32_t xg_index);
void switch_slave_global_flowcontrol_threshold_set(uint32_t xon_threshold, uint32_t xoff_threshold);
void switch_slave_port_flowcontrol_threshold_set(uint32_t port_id, uint32_t xon_threshold, uint32_t xoff_threshold);

void switch_slave_port_flowcontrol_mode_set(uint32_t port_id, uint32_t enable);

void switch_slave_fwd_dev_id_set(uint32_t dev_id);

void swith_slave_init(void);

void switch_master_stk_flowcontrol_mode_set(uint32_t enable);
void switch_slave_stk_flowcontrol_mode_set(uint32_t enable);

void switch_master_stkport_fc_thd_set(uint32_t xon_thd, uint32_t xoff_thd);
void switch_slave_stkport_fc_thd_set(uint32_t xon_thd, uint32_t xoff_thd);
void switch_master_24ports_fc_thd_set(uint32_t xon_thd, uint32_t xoff_thd);
void switch_slave_24ports_fc_thd_set(uint32_t xon_thd, uint32_t xoff_thd);
void switch_master_stk_queue_holb1_set(uint32_t value1);
void switch_slave_stk_queue_holb1_set(uint32_t value1);
void switch_master_stk_queue_holb2_set(uint32_t value2);
void switch_slave_stk_queue_holb2_set(uint32_t value2);
void switch_master_24ports_queue_holb1_set(uint32_t value1);
void switch_slave_24ports_queue_holb1_set(uint32_t value1);
void switch_master_24ports_queue_holb2_set(uint32_t value2);
void switch_slave_24ports_queue_holb2_set(uint32_t value2);
void switch_master_24ports_queue_holb3_set(uint32_t value3);
void switch_slave_24ports_queue_holb3_set(uint32_t value3);
void switch_master_stk_wred_set(uint32_t enable);
void switch_slave_stk_wred_set(uint32_t enable);
void switch_master_24ports_wred_set(uint32_t enable);
void switch_slave_24ports_wred_set(uint32_t enable);
void switch_master_stk_queue_holb3_set(uint32_t value3);
void switch_slave_stk_queue_holb3_set(uint32_t value3);

#endif
