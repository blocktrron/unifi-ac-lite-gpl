#include <asm/addrspace.h>
#include <asm/types.h>
#include <linux/types.h>
#include <config.h>
#include <common.h>
#include <music_soc.h>
#include "music_i2c.h"

void i2c_hw_init(void)
{
    union i2c_si_config_reg i2c_si_init;
    union i2c_si_cs_reg     i2c_cs_init;

    memset(&i2c_si_init, 0, sizeof(i2c_si_init));
    memset(&i2c_cs_init, 0, sizeof(i2c_cs_init));

    i2c_si_init.val = i2c_reg_rd(I2C_SI_CONFIG_REG);

    /* set SI to i2c mode */
    i2c_si_init.content.I2C            = 1;
    i2c_si_init.content.INACTIVE_DATA  = 1;
    i2c_si_init.content.INACTIVE_CLK   = 1;
    i2c_si_init.content.DIVIDER        = 0xb;

    i2c_reg_wr(I2C_SI_CONFIG_REG, i2c_si_init.val);

    /* clear cs DONE_INT bit */
    i2c_cs_init.val = i2c_reg_rd(I2C_SI_CS_REG);
    i2c_cs_init.content.DONE_INT = 1;
    i2c_reg_wr(I2C_SI_CS_REG, i2c_cs_init.val);
}

uint32_t _i2c_read(uint32_t chip, uint32_t addr, uint32_t alen,
						uint8_t *buf, uint32_t count)
{
    uint32_t i = 0, error = 0;
    union i2c_si_cs_reg i2c_cs_read = {0};
	uint8_t  tx_data[8] = {0};
	uint32_t data_hi = 0, data_lo = 0;

    if((NULL == buf) || (0 == count) || (count > 8)) {
        printf("i2c_read: wrong parameter buf=%x count=%d\n",
				(uint32_t)buf, count);
        error = 1;
        return error;
    }

    /*(1) send address*/
	tx_data[0] = (chip<<1) | I2C_WRITE;

	for(i = 1; i < alen+1; i++) {
		tx_data[i] = (addr >> (8*(alen - i)))& 0xFF;
	}
	tx_data[i] = (chip << 1) | I2C_READ;
    i2c_reg_wr(I2C_SI_TX_DATA0_REG, SWAP32(*(uint32_t *)&tx_data[0]));
	i2c_reg_wr(I2C_SI_TX_DATA1_REG, SWAP32(*(uint32_t *)&tx_data[4]));

    /*(2)trigger start*/
    i2c_cs_read.val = i2c_reg_rd(I2C_SI_CS_REG);
    i2c_cs_read.content.RX_CNT = count;
    i2c_cs_read.content.TX_CNT = (1+1+alen);
    i2c_cs_read.content.START  = 1;
    i2c_reg_wr(I2C_SI_CS_REG, i2c_cs_read.val);

    /*(3) polling done*/
    i2c_cs_read.val = i2c_reg_rd(I2C_SI_CS_REG);
    while(!(i2c_cs_read.content.DONE_INT)) {
        i2c_cs_read.val = i2c_reg_rd(I2C_SI_CS_REG);
        if(i2c_cs_read.content.DONE_ERR) {
            printf("i2c_read: chip=%x addr=%x fail\n",
							chip, addr);
            error = 1;
            break;
        }
    }

    /* (4)clear cs DONE_INT bit */
    i2c_cs_read.val = i2c_reg_rd(I2C_SI_CS_REG);
    i2c_cs_read.content.DONE_INT = 1;
    i2c_reg_wr(I2C_SI_CS_REG, i2c_cs_read.val);
    if(error) {
        return error;
    }

    /* (5)read content*/
    data_lo = i2c_reg_rd(I2C_SI_RX_DATA0_REG);
    data_hi = i2c_reg_rd(I2C_SI_RX_DATA1_REG);

	memset(buf, 0, count);
    for(i = 0; i < count; i ++) {
        if(i < 4) {
            buf[i] = ((data_lo >> (8 * i)) & 0xFF);
        } else if(i < 8) {
            buf[i] = ((data_hi >> (8 * (i - 4))) & 0xFF);
        }
    }

    return error;
}

uint32_t i2c_read(uint32_t chip, uint32_t addr, uint32_t alen,
									uint8_t *buf, uint32_t count)
{
	uint32_t i, offset, error, i2c_max_per_rd = 8;

	i2c_hw_init();

	for(i = 0; i < (count/i2c_max_per_rd); i++) {
		offset = i*i2c_max_per_rd;
		error = _i2c_read(chip, (addr+offset), alen,
						 (buf+offset), (i2c_max_per_rd));
		if(error) {
			return error;
		}
	}

	if(count%i2c_max_per_rd) {
		offset = i*i2c_max_per_rd;
		error = _i2c_read(chip, (addr+offset), alen,
						(buf+offset), (count%i2c_max_per_rd));
		if(error) {
			return error;
		}
	}

	return 0;
}

uint32_t i2c_write(uint32_t chip, uint32_t addr, uint32_t alen,
					uint8_t *buf, uint32_t count)
{
    uint32_t i = 0, j = 0, error = 0;
    union i2c_si_cs_reg i2c_cs_write = {0};
	uint8_t  tx_data[8] = {0};

	i2c_hw_init();

    if((NULL == buf) || (0 == count) || ((count+alen) > 7)) {
        printf("i2c_write:wrong parameter buf=%x count =%d\n",
                (uint32_t)buf, count);
		error = 1;
        return error;
    }

    /*(1) send address&data*/
	tx_data[0] = (chip<<1) | I2C_WRITE;

	for(i = 1; i< alen+1; i++) {
		tx_data[i] = (addr >> (8*(alen - i)))& 0xFF;
	}

	for(j = 0; j < count; j++) {
		tx_data[i+j] = buf[j];
	}
    i2c_reg_wr(I2C_SI_TX_DATA0_REG, SWAP32(*(uint32_t *)&tx_data[0]));
	i2c_reg_wr(I2C_SI_TX_DATA1_REG, SWAP32(*(uint32_t *)&tx_data[4]));

    /*(2)trigger start*/
    i2c_cs_write.val = i2c_reg_rd(I2C_SI_CS_REG);
    i2c_cs_write.content.RX_CNT    = 0;
    i2c_cs_write.content.TX_CNT    = 1 + alen + count;
    i2c_cs_write.content.START     = 1;
    i2c_cs_write.content.DONE_INT  = 1;
    i2c_reg_wr(I2C_SI_CS_REG, i2c_cs_write.val);

	/*(3) polling done*/
    i2c_cs_write.val = i2c_reg_rd(I2C_SI_CS_REG);
    while(!(i2c_cs_write.content.DONE_INT)) {
        i2c_cs_write.val = i2c_reg_rd(I2C_SI_CS_REG);
        if(i2c_cs_write.content.DONE_ERR) {
            printf("i2c_write: chip=%x addr=%x fail\n",
							chip, addr);
            error = 1;
            break;
        }
    }

    /*(4) clear cs DONE_INT bit */
    i2c_cs_write.val = i2c_reg_rd(I2C_SI_CS_REG);
    i2c_cs_write.content.DONE_INT = 1;
    i2c_reg_wr(I2C_SI_CS_REG, i2c_cs_write.val);

    udelay(100*1000);

    return error;
}

int i2c_probe(uint32_t chip)
{
	uint32_t addr = 0;
	uint32_t alen = 2;
	uint32_t data = 0;
	uint32_t count = 4;

	return (i2c_read(chip, addr, alen, (uint8_t *)&data, count) == 0);
}
