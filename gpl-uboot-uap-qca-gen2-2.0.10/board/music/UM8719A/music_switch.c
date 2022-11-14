#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <config.h>
#include <version.h>
#include "music_soc.h"

#define MUSIC_SWITCH_BASE 0xb8800000
/*clear phy reset*/
#define REG_MAC_XMII_CTRL   0x0008
#define MAC_XMII_PHY_RST_EN_CFG_MASK 0x01000000
/*PORT_STATUS*/
#define REG_PORT_STATUS	    0x100

#define music_switch_wr(addr, val)  music_reg_wr(MUSIC_SWITCH_BASE + addr, val)
#define music_switch_rd(addr)       music_reg_rd(MUSIC_SWITCH_BASE + addr)

static void
music_switch_phy_reset_enable(void)
{
    unsigned int reg_addr = REG_MAC_XMII_CTRL, reg_val;

    /*clear phy reset*/
    reg_val = music_switch_rd(reg_addr);
    reg_val |= (MAC_XMII_PHY_RST_EN_CFG_MASK);
    music_switch_wr(reg_addr, reg_val);

    //udelay(4*1000*1000);
}

static void
music_switch_phy_reset_release(void)
{
    unsigned int reg_addr = REG_MAC_XMII_CTRL, reg_val;

    /*clear phy reset*/
    reg_val = music_switch_rd(reg_addr);
    reg_val &= (~MAC_XMII_PHY_RST_EN_CFG_MASK);
    music_switch_wr(reg_addr, reg_val);

    //udelay(4*1000*1000);
}

static void
music_switch_mac_enable(void)
{
    unsigned int  reg_addr;
    unsigned int port;

    for(port = 0; port <24; port++) {
       reg_addr = REG_PORT_STATUS+ port*0x100;
       music_switch_wr(reg_addr, 0x000012c2);
    }
}

static void
music_switch_mac_disable(void)
{
    unsigned int  reg_addr;
    unsigned int port;

    for(port = 0; port <24; port++) {
       reg_addr = REG_PORT_STATUS+ port*0x100;
       music_switch_wr(reg_addr, 0);
    }
}

static void
music_switch_mac_func(void)
{
    music_switch_wr(0x1c, 0x01fffffe);
    music_switch_wr(0x15010, 0x90ffffff);
}

void
music_switch_phy_init(void)
{
    music_switch_wr(0x15004, 0x800d0007);udelay(100);
    music_switch_wr(0x15004, 0x800e801a);udelay(100);
    music_switch_wr(0x15004, 0x800d4007);udelay(100);
    music_switch_wr(0x15004, 0x800e592a);udelay(100);
    music_switch_wr(0x15004, 0x802d0007);udelay(100);
    music_switch_wr(0x15004, 0x802e801a);udelay(100);
    music_switch_wr(0x15004, 0x802d4007);udelay(100);
    music_switch_wr(0x15004, 0x802e592a);udelay(100);
    music_switch_wr(0x15004, 0x804d0007);udelay(100);
    music_switch_wr(0x15004, 0x804e801a);udelay(100);
    music_switch_wr(0x15004, 0x804d4007);udelay(100);
    music_switch_wr(0x15004, 0x804e592a);udelay(100);
    music_switch_wr(0x15004, 0x806d0007);udelay(100);
    music_switch_wr(0x15004, 0x806e801a);udelay(100);
    music_switch_wr(0x15004, 0x806d4007);udelay(100);
    music_switch_wr(0x15004, 0x806e592a);udelay(100);
    music_switch_wr(0x15004, 0x808d0007);udelay(100);
    music_switch_wr(0x15004, 0x808e801a);udelay(100);
    music_switch_wr(0x15004, 0x808d4007);udelay(100);
    music_switch_wr(0x15004, 0x808e592a);udelay(100);
    music_switch_wr(0x15004, 0x80ad0007);udelay(100);
    music_switch_wr(0x15004, 0x80ae801a);udelay(100);
    music_switch_wr(0x15004, 0x80ad4007);udelay(100);
    music_switch_wr(0x15004, 0x80ae592a);udelay(100);
    music_switch_wr(0x15004, 0x80cd0007);udelay(100);
    music_switch_wr(0x15004, 0x80ce801a);udelay(100);
    music_switch_wr(0x15004, 0x80cd4007);udelay(100);
    music_switch_wr(0x15004, 0x80ce592a);udelay(100);
    music_switch_wr(0x15004, 0x80ed0007);udelay(100);
    music_switch_wr(0x15004, 0x80ee801a);udelay(100);
    music_switch_wr(0x15004, 0x80ed4007);udelay(100);
    music_switch_wr(0x15004, 0x80ee592a);udelay(100);

    music_switch_wr(0x15004, 0x910d0007);udelay(100);
    music_switch_wr(0x15004, 0x910e801a);udelay(100);
    music_switch_wr(0x15004, 0x910d4007);udelay(100);
    music_switch_wr(0x15004, 0x910e592a);udelay(100);
    music_switch_wr(0x15004, 0x912d0007);udelay(100);
    music_switch_wr(0x15004, 0x912e801a);udelay(100);
    music_switch_wr(0x15004, 0x912d4007);udelay(100);
    music_switch_wr(0x15004, 0x912e592a);udelay(100);
    music_switch_wr(0x15004, 0x914d0007);udelay(100);
    music_switch_wr(0x15004, 0x914e801a);udelay(100);
    music_switch_wr(0x15004, 0x914d4007);udelay(100);
    music_switch_wr(0x15004, 0x914e592a);udelay(100);
    music_switch_wr(0x15004, 0x916d0007);udelay(100);
    music_switch_wr(0x15004, 0x916e801a);udelay(100);
    music_switch_wr(0x15004, 0x916d4007);udelay(100);
    music_switch_wr(0x15004, 0x916e592a);udelay(100);
    music_switch_wr(0x15004, 0x918d0007);udelay(100);
    music_switch_wr(0x15004, 0x918e801a);udelay(100);
    music_switch_wr(0x15004, 0x918d4007);udelay(100);
    music_switch_wr(0x15004, 0x918e592a);udelay(100);
    music_switch_wr(0x15004, 0x91ad0007);udelay(100);
    music_switch_wr(0x15004, 0x91ae801a);udelay(100);
    music_switch_wr(0x15004, 0x91ad4007);udelay(100);
    music_switch_wr(0x15004, 0x91ae592a);udelay(100);
    music_switch_wr(0x15004, 0x91cd0007);udelay(100);
    music_switch_wr(0x15004, 0x91ce801a);udelay(100);
    music_switch_wr(0x15004, 0x91cd4007);udelay(100);
    music_switch_wr(0x15004, 0x91ce592a);udelay(100);
    music_switch_wr(0x15004, 0x91ed0007);udelay(100);
    music_switch_wr(0x15004, 0x91ee801a);udelay(100);
    music_switch_wr(0x15004, 0x91ed4007);udelay(100);
    music_switch_wr(0x15004, 0x91ee592a);udelay(100);

    music_switch_wr(0x15004, 0xa20d0007);udelay(100);
    music_switch_wr(0x15004, 0xa20e801a);udelay(100);
    music_switch_wr(0x15004, 0xa20d4007);udelay(100);
    music_switch_wr(0x15004, 0xa20e592a);udelay(100);
    music_switch_wr(0x15004, 0xa22d0007);udelay(100);
    music_switch_wr(0x15004, 0xa22e801a);udelay(100);
    music_switch_wr(0x15004, 0xa22d4007);udelay(100);
    music_switch_wr(0x15004, 0xa22e592a);udelay(100);
    music_switch_wr(0x15004, 0xa24d0007);udelay(100);
    music_switch_wr(0x15004, 0xa24e801a);udelay(100);
    music_switch_wr(0x15004, 0xa24d4007);udelay(100);
    music_switch_wr(0x15004, 0xa24e592a);udelay(100);
    music_switch_wr(0x15004, 0xa26d0007);udelay(100);
    music_switch_wr(0x15004, 0xa26e801a);udelay(100);
    music_switch_wr(0x15004, 0xa26d4007);udelay(100);
    music_switch_wr(0x15004, 0xa26e592a);udelay(100);
    music_switch_wr(0x15004, 0xa28d0007);udelay(100);
    music_switch_wr(0x15004, 0xa28e801a);udelay(100);
    music_switch_wr(0x15004, 0xa28d4007);udelay(100);
    music_switch_wr(0x15004, 0xa28e592a);udelay(100);
    music_switch_wr(0x15004, 0xa2ad0007);udelay(100);
    music_switch_wr(0x15004, 0xa2ae801a);udelay(100);
    music_switch_wr(0x15004, 0xa2ad4007);udelay(100);
    music_switch_wr(0x15004, 0xa2ae592a);udelay(100);
    music_switch_wr(0x15004, 0xa2cd0007);udelay(100);
    music_switch_wr(0x15004, 0xa2ce801a);udelay(100);
    music_switch_wr(0x15004, 0xa2cd4007);udelay(100);
    music_switch_wr(0x15004, 0xa2ce592a);udelay(100);
    music_switch_wr(0x15004, 0xa2ed0007);udelay(100);
    music_switch_wr(0x15004, 0xa2ee801a);udelay(100);
    music_switch_wr(0x15004, 0xa2ed4007);udelay(100);
    music_switch_wr(0x15004, 0xa2ee592a);udelay(100);

    music_switch_wr(0x15004, 0x800d0003);udelay(100);
    music_switch_wr(0x15004, 0x800e8007);udelay(100);
    music_switch_wr(0x15004, 0x800d4003);udelay(100);
    music_switch_wr(0x15004, 0x800efff6);udelay(100);
    music_switch_wr(0x15004, 0x802d0003);udelay(100);
    music_switch_wr(0x15004, 0x802e8007);udelay(100);
    music_switch_wr(0x15004, 0x802d4003);udelay(100);
    music_switch_wr(0x15004, 0x802efff6);udelay(100);
    music_switch_wr(0x15004, 0x804d0003);udelay(100);
    music_switch_wr(0x15004, 0x804e8007);udelay(100);
    music_switch_wr(0x15004, 0x804d4003);udelay(100);
    music_switch_wr(0x15004, 0x804efff6);udelay(100);
    music_switch_wr(0x15004, 0x806d0003);udelay(100);
    music_switch_wr(0x15004, 0x806e8007);udelay(100);
    music_switch_wr(0x15004, 0x806d4003);udelay(100);
    music_switch_wr(0x15004, 0x806efff6);udelay(100);
    music_switch_wr(0x15004, 0x808d0003);udelay(100);
    music_switch_wr(0x15004, 0x808e8007);udelay(100);
    music_switch_wr(0x15004, 0x808d4003);udelay(100);
    music_switch_wr(0x15004, 0x808efff6);udelay(100);
    music_switch_wr(0x15004, 0x80ad0003);udelay(100);
    music_switch_wr(0x15004, 0x80ae8007);udelay(100);
    music_switch_wr(0x15004, 0x80ad4003);udelay(100);
    music_switch_wr(0x15004, 0x80aefff6);udelay(100);
    music_switch_wr(0x15004, 0x80cd0003);udelay(100);
    music_switch_wr(0x15004, 0x80ce8007);udelay(100);
    music_switch_wr(0x15004, 0x80cd4003);udelay(100);
    music_switch_wr(0x15004, 0x80cefff6);udelay(100);
    music_switch_wr(0x15004, 0x80ed0003);udelay(100);
    music_switch_wr(0x15004, 0x80ee8007);udelay(100);
    music_switch_wr(0x15004, 0x80ed4003);udelay(100);
    music_switch_wr(0x15004, 0x80eefff6);udelay(100);
    music_switch_wr(0x15004, 0x80009000);udelay(100);
    music_switch_wr(0x15004, 0x80209000);udelay(100);
    music_switch_wr(0x15004, 0x80409000);udelay(100);
    music_switch_wr(0x15004, 0x80609000);udelay(100);
    music_switch_wr(0x15004, 0x80809000);udelay(100);
    music_switch_wr(0x15004, 0x80a09000);udelay(100);
    music_switch_wr(0x15004, 0x80c09000);udelay(100);
    music_switch_wr(0x15004, 0x80e09000);udelay(100);

    music_switch_wr(0x15004, 0x910d0003);udelay(100);
    music_switch_wr(0x15004, 0x910e8007);udelay(100);
    music_switch_wr(0x15004, 0x910d4003);udelay(100);
    music_switch_wr(0x15004, 0x910efff6);udelay(100);
    music_switch_wr(0x15004, 0x912d0003);udelay(100);
    music_switch_wr(0x15004, 0x912e8007);udelay(100);
    music_switch_wr(0x15004, 0x912d4003);udelay(100);
    music_switch_wr(0x15004, 0x912efff6);udelay(100);
    music_switch_wr(0x15004, 0x914d0003);udelay(100);
    music_switch_wr(0x15004, 0x914e8007);udelay(100);
    music_switch_wr(0x15004, 0x914d4003);udelay(100);
    music_switch_wr(0x15004, 0x914efff6);udelay(100);
    music_switch_wr(0x15004, 0x916d0003);udelay(100);
    music_switch_wr(0x15004, 0x916e8007);udelay(100);
    music_switch_wr(0x15004, 0x916d4003);udelay(100);
    music_switch_wr(0x15004, 0x916efff6);udelay(100);
    music_switch_wr(0x15004, 0x918d0003);udelay(100);
    music_switch_wr(0x15004, 0x918e8007);udelay(100);
    music_switch_wr(0x15004, 0x918d4003);udelay(100);
    music_switch_wr(0x15004, 0x918efff6);udelay(100);
    music_switch_wr(0x15004, 0x91ad0003);udelay(100);
    music_switch_wr(0x15004, 0x91ae8007);udelay(100);
    music_switch_wr(0x15004, 0x91ad4003);udelay(100);
    music_switch_wr(0x15004, 0x91aefff6);udelay(100);
    music_switch_wr(0x15004, 0x91cd0003);udelay(100);
    music_switch_wr(0x15004, 0x91ce8007);udelay(100);
    music_switch_wr(0x15004, 0x91cd4003);udelay(100);
    music_switch_wr(0x15004, 0x91cefff6);udelay(100);
    music_switch_wr(0x15004, 0x91ed0003);udelay(100);
    music_switch_wr(0x15004, 0x91ee8007);udelay(100);
    music_switch_wr(0x15004, 0x91ed4003);udelay(100);
    music_switch_wr(0x15004, 0x91eefff6);udelay(100);
    music_switch_wr(0x15004, 0x91009000);udelay(100);
    music_switch_wr(0x15004, 0x91209000);udelay(100);
    music_switch_wr(0x15004, 0x91409000);udelay(100);
    music_switch_wr(0x15004, 0x91609000);udelay(100);
    music_switch_wr(0x15004, 0x91809000);udelay(100);
    music_switch_wr(0x15004, 0x91a09000);udelay(100);
    music_switch_wr(0x15004, 0x91c09000);udelay(100);
    music_switch_wr(0x15004, 0x91e09000);udelay(100);

    music_switch_wr(0x15004, 0xa20d0003);udelay(100);
    music_switch_wr(0x15004, 0xa20e8007);udelay(100);
    music_switch_wr(0x15004, 0xa20d4003);udelay(100);
    music_switch_wr(0x15004, 0xa20efff6);udelay(100);
    music_switch_wr(0x15004, 0xa22d0003);udelay(100);
    music_switch_wr(0x15004, 0xa22e8007);udelay(100);
    music_switch_wr(0x15004, 0xa22d4003);udelay(100);
    music_switch_wr(0x15004, 0xa22efff6);udelay(100);
    music_switch_wr(0x15004, 0xa24d0003);udelay(100);
    music_switch_wr(0x15004, 0xa24e8007);udelay(100);
    music_switch_wr(0x15004, 0xa24d4003);udelay(100);
    music_switch_wr(0x15004, 0xa24efff6);udelay(100);
    music_switch_wr(0x15004, 0xa26d0003);udelay(100);
    music_switch_wr(0x15004, 0xa26e8007);udelay(100);
    music_switch_wr(0x15004, 0xa26d4003);udelay(100);
    music_switch_wr(0x15004, 0xa26efff6);udelay(100);
    music_switch_wr(0x15004, 0xa28d0003);udelay(100);
    music_switch_wr(0x15004, 0xa28e8007);udelay(100);
    music_switch_wr(0x15004, 0xa28d4003);udelay(100);
    music_switch_wr(0x15004, 0xa28efff6);udelay(100);
    music_switch_wr(0x15004, 0xa2ad0003);udelay(100);
    music_switch_wr(0x15004, 0xa2ae8007);udelay(100);
    music_switch_wr(0x15004, 0xa2ad4003);udelay(100);
    music_switch_wr(0x15004, 0xa2aefff6);udelay(100);
    music_switch_wr(0x15004, 0xa2cd0003);udelay(100);
    music_switch_wr(0x15004, 0xa2ce8007);udelay(100);
    music_switch_wr(0x15004, 0xa2cd4003);udelay(100);
    music_switch_wr(0x15004, 0xa2cefff6);udelay(100);
    music_switch_wr(0x15004, 0xa2ed0003);udelay(100);
    music_switch_wr(0x15004, 0xa2ee8007);udelay(100);
    music_switch_wr(0x15004, 0xa2ed4003);udelay(100);
    music_switch_wr(0x15004, 0xa2eefff6);udelay(100);
    music_switch_wr(0x15004, 0xa2009000);udelay(100);
    music_switch_wr(0x15004, 0xa2209000);udelay(100);
    music_switch_wr(0x15004, 0xa2409000);udelay(100);
    music_switch_wr(0x15004, 0xa2609000);udelay(100);
    music_switch_wr(0x15004, 0xa2809000);udelay(100);
    music_switch_wr(0x15004, 0xa2a09000);udelay(100);
    music_switch_wr(0x15004, 0xa2c09000);udelay(100);
    music_switch_wr(0x15004, 0xa2e09000);udelay(100);
}

#define GLOBAL_CTRL_0 0
#define GLOBAL_CTRL_0_SOFT_RST_MASK 	0x80000000
#define GLOBAL_CTRL_0_REG_RST_EN_MASK 	0x00040000

void
music_switch_soft_reset(void)
{
	unsigned int reg_addr = GLOBAL_CTRL_0, reg_val;

	reg_val = music_switch_rd(reg_addr);
	reg_val |= (GLOBAL_CTRL_0_SOFT_RST_MASK	|
				GLOBAL_CTRL_0_REG_RST_EN_MASK);
	music_switch_wr(reg_addr, reg_val);

	printf("switch reset done\n");
}

void
music_switch_main(void)
{
	music_switch_soft_reset();
    music_switch_mac_disable();
    music_switch_phy_reset_release();
    music_switch_phy_init();
    music_switch_mac_func();
    music_switch_mac_enable();

    printf("sw init done\n");
}
