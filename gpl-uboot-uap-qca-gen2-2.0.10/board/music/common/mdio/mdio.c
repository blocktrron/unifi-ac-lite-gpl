#include "mdio.h"

static int32_t
mdio_write(uint32_t addr, uint32_t data,
                  uint32_t mode, uint32_t preamble, uint32_t dev_sel)
{
    uint32_t i = 100;
    uint32_t val = 0;

    if (mode == MUSIC_MDIOM_ENHANCE_MODE) { /*enhance */
        MDIOM_WRITE_REG(REG_MDIOM_ADDR, (addr & MDIOM_REGAD_VAL_MASK));

    } else {                    /*single */
        /*addr high */
        MDIOM_WRITE_REG(REG_MDIOM_PHYAD, ((addr >> 5) & MDIOM_PHYAD_VAL_MASK));

        /*addr low */
        MDIOM_WRITE_REG(REG_MDIOM_REGAD, (addr & MDIOM_REGAD_VAL_MASK));
    }

    /*data */
    MDIOM_WRITE_REG(REG_MDIOM_DATA, data);

    /*start */
    val = (((preamble << MDIOM_CTRL_SUP_PRE_SHIFT) & MDIOM_CTRL_SUP_PRE_MASK) |
           ((mode << MDIOM_CTRL_SIG_MODE_SHIFT) & MDIOM_CTRL_SIG_MODE_MASK) |
           MDIOM_CTRL_WR_START_MASK | MDIOM_CTRL_ENABLE_MASK);

    if(dev_sel == MUSIC_MDIOM_DEV_SEL_SWITCH) {
        val |=  MDIOM_CTRL_DEV_SEL_SWITCH;
    }

    MDIOM_WRITE_REG(REG_MDIOM_CTRL, val);

    /*polling done */
    while ((MDIOM_READ_REG(REG_MDIOM_STATUS) & MDIOM_STATUS_BUSY_MASK) && (i--)) {
        udelay(1000);
    }

    if (i == 0) {
        return -1;
    }

    return 0;
}

static int32_t
mdio_read(uint32_t addr, uint32_t * data,
              uint32_t mode, uint32_t preamble, uint32_t dev_sel)
{
    uint32_t i = 100;
    uint32_t val = 0;

    if (mode == MUSIC_MDIOM_ENHANCE_MODE) { /*enhance */
        MDIOM_WRITE_REG(REG_MDIOM_ADDR, (addr & MDIOM_REGAD_VAL_MASK));

    } else {                    /*single */
        /*addr high */
        MDIOM_WRITE_REG(REG_MDIOM_PHYAD, ((addr >> 5) & MDIOM_PHYAD_VAL_MASK));

        /*addr low */
        MDIOM_WRITE_REG(REG_MDIOM_REGAD, (addr & MDIOM_REGAD_VAL_MASK));
    }

    /*data */
    MDIOM_WRITE_REG(REG_MDIOM_DATA, 0);

    /*start */
    val = (((preamble << MDIOM_CTRL_SUP_PRE_SHIFT) & MDIOM_CTRL_SUP_PRE_MASK) |
           ((mode << MDIOM_CTRL_SIG_MODE_SHIFT) & MDIOM_CTRL_SIG_MODE_MASK) |
           MDIOM_CTRL_RD_START_MASK | MDIOM_CTRL_ENABLE_MASK);
    if(dev_sel == MUSIC_MDIOM_DEV_SEL_SWITCH) {
        val |=  MDIOM_CTRL_DEV_SEL_SWITCH;
    }

    MDIOM_WRITE_REG(REG_MDIOM_CTRL, val);

    /*polling done */
    while ((MDIOM_READ_REG(REG_MDIOM_STATUS) & MDIOM_STATUS_BUSY_MASK) && (i--)) {
		udelay(1000);
    }

    if (i == 0) {
        return -1;
    }

    *data = MDIOM_READ_REG(REG_MDIOM_DATA);

    return 0;
}

int32_t
mdio_switch_reg_wr(uint32_t addr, uint32_t data)
{
    return mdio_write(addr, data,
                             MUSIC_MDIOM_ENHANCE_MODE,
                             MUSIC_MDIOM_NO_SUPPRESS_PREAMBLE,
                             MUSIC_MDIOM_DEV_SEL_SWITCH);
}

uint32_t
mdio_switch_reg_rd(uint32_t addr)
{
	uint32_t data = 0;

	mdio_read(addr, &data,
	                    MUSIC_MDIOM_ENHANCE_MODE,
	                    MUSIC_MDIOM_NO_SUPPRESS_PREAMBLE,
	                    MUSIC_MDIOM_DEV_SEL_SWITCH);
	return data;
}

int32_t
mdio_soc_reg_wr(uint32_t addr, uint32_t data)
{
	/*we need internal offset rather than mips address*/
	if((addr & 0xa0000000) == 0xa0000000) {
		addr = (addr & (~0xa0000000));

	} else if((addr & 0x80000000) == 0x80000000) {
		addr = (addr & (~0x80000000));
	}

	if(MUSIC_REG_IS_SWITCH(addr)) {
		return mdio_switch_reg_wr(addr, data);

	} else {
	    return mdio_write(addr, data,
	                             MUSIC_MDIOM_ENHANCE_MODE,
	                             MUSIC_MDIOM_NO_SUPPRESS_PREAMBLE,
	                             MUSIC_MDIOM_DEV_SEL_SOC);
	}
}

uint32_t
mdio_soc_reg_rd(uint32_t addr)
{
	uint32_t data = 0;

	/*we need internal offset rather than mips address*/
	if((addr & 0xa0000000) == 0xa0000000) {
		addr = (addr & (~0xa0000000));

	} else if((addr & 0x80000000) == 0x80000000) {
		addr = (addr & (~0x80000000));
	}

	if(MUSIC_REG_IS_SWITCH(addr)) {
		data = mdio_switch_reg_rd(addr);

	} else {
		mdio_read(addr, &data,
                            MUSIC_MDIOM_ENHANCE_MODE,
                            MUSIC_MDIOM_NO_SUPPRESS_PREAMBLE,
                            MUSIC_MDIOM_DEV_SEL_SOC);
	}

	return data;
}

void
mdio_hw_init(void)
{
    /*set mdio default op code */
    uint32_t op_code_rd = 2;
    uint32_t op_code_wr = 1;
    uint32_t op_code_start = 1;

    /*mdc clock frequency: AHB_CLK/(DIVIDER*4) */
    MDIOM_WRITE_REG(REG_MDIOM_CLK_DIVIDER,
				(0xa & MDIOM_CLK_DIVIDER_VAL_MASK));
    MDIOM_WRITE_REG(REG_MDIOM_ST_OP, ((op_code_rd << 4) |
                                      (op_code_wr << 2) |
                                      (op_code_start)));

    printf("mdiom init done\n");
}
