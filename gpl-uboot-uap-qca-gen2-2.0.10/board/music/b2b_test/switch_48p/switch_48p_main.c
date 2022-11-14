#include "switch_48p.h"
#include "switch_48p_api.h"

void switch_48p_main(void)
{
    /*port1-24 alpha value*/
	switch_master_24ports_wred_set(1);
	switch_slave_24ports_wred_set(1);
	switch_master_24ports_queue_holb1_set(0x02400240);
	switch_slave_24ports_queue_holb1_set(0x02400240);
	switch_master_24ports_queue_holb2_set(0x02400240);
	switch_slave_24ports_queue_holb2_set(0x02400240);
	switch_master_24ports_queue_holb3_set(0x04000003);
	switch_slave_24ports_queue_holb3_set(0x04000003);

    /*stack init*/
	master_switch_stack_init();
	slave_switch_stack_init();

    /*stack port alpha value*/
	switch_master_stk_wred_set(1);
	switch_slave_stk_wred_set(1);
	switch_master_stk_queue_holb1_set(0x02c002c0);
	switch_slave_stk_queue_holb1_set(0x02c002c0);
	switch_master_stk_queue_holb2_set(0x02c002c0);
	switch_slave_stk_queue_holb2_set(0x02c002c0);

	printf("b2b init done\n");
}
