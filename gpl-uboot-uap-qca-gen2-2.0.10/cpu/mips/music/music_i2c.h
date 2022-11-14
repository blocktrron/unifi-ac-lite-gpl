#ifndef _MUSIC_I2C_H_
#define _MUSIC_I2C_H_

/*eeprom*/
#define I2C_BUS_MAX             0x80
#define EEPROM_SIZE_MAX         0xFFFF

/*register define*/
#define I2C_SI_CONFIG_REG      0x00000000
#define I2C_SI_CS_REG          0x00000004
#define I2C_SI_TX_DATA0_REG    0x00000008
#define I2C_SI_TX_DATA1_REG    0x0000000c
#define I2C_SI_RX_DATA0_REG    0x00000010
#define I2C_SI_RX_DATA1_REG    0x00000014


#define I2C_READ           1
#define I2C_WRITE          0

#define I2C_REG_BASE 0x18090000

#define i2c_reg_rd(r)	({		\
	music_reg_rd(I2C_REG_BASE + (r));	\
})

#define i2c_reg_wr(r, v)	({	\
	music_reg_wr((I2C_REG_BASE + (r)), v);	\
})

#define SWAP32(x)	 ((((x) & 0x000000ff) << 24) | (((x) & 0x0000ff00) << 8)|\
			  (((x) & 0x00ff0000) >>  8) | (((x) & 0xff000000) >> 24) )

union i2c_si_config_reg{
    uint32_t val;
    struct {
        uint32_t RESERVER3:12;
        uint32_t ERR_INT:1;
        uint32_t DIDIR_OD_DATA:1;
        uint32_t RESERVER2:1;
        uint32_t I2C:1;
        uint32_t RESERVER1:8;
        uint32_t POS_SAMPLE:1;
        uint32_t POS_DRIVE:1;
        uint32_t INACTIVE_DATA:1;
        uint32_t INACTIVE_CLK:1;
        uint32_t DIVIDER:4;
    }content;
};

union i2c_si_cs_reg{
    uint32_t val;
    struct {
        uint32_t RESERVER3:18;
        uint32_t BIT_CNT_IN_LAST_BYTE:3;
        uint32_t DONE_ERR:1;
        uint32_t DONE_INT:1;
        uint32_t START:1;
        uint32_t RX_CNT:4;
        uint32_t TX_CNT:4;
    }content;
};

#endif
