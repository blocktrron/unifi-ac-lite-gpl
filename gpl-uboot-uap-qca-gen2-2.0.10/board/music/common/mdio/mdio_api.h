#ifndef _MDIO_API_H_
#define _MDIO_API_H_

int32_t mdio_switch_reg_wr(uint32_t addr, uint32_t data);
uint32_t mdio_switch_reg_rd(uint32_t addr);
int32_t mdio_soc_reg_wr(uint32_t addr, uint32_t data);
uint32_t mdio_soc_reg_rd(uint32_t addr);
void mdio_hw_init(void);

#endif /*_MDIO_API_H_*/
