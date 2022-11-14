#ifndef _ATHR8035_PHY_H
#define _ATHR8035_PHY_H


/*****************/
/* PHY Registers */
/*****************/
#ifndef ATHR_PHY_CONTROL
#define ATHR_PHY_CONTROL                 0x00
#define ATHR_PHY_STATUS                  0x01
#define ATHR_PHY_ID1                     0x02
#define ATHR_PHY_ID2                     0x03
#define ATHR_AUTONEG_ADVERT              0x04
#define ATHR_LINK_PARTNER_ABILITY        0x05
#define ATHR_AUTONEG_EXPANSION           0x06
#define ATHR_NEXT_PAGE_TRANSMIT          0x07
#define ATHR_LINK_PARTNER_NEXT_PAGE      0x08
#define ATHR_1000BASET_CONTROL           0x09
#define ATHR_1000BASET_STATUS            0x0a
#define ATHR_PHY_SPEC_CONTROL            0x10
#define ATHR_PHY_SPEC_STATUS             0x11
#define ATHR_INTERRUPT_ENABLE		 0x12
#define ATHR_INTERRUPT_STATUS            0x13
#define ATHR_EXTENDED_PHY_SPEC_CONTROL   0x14
#define ATHR_RECV_ERROR_COUNTER          0x15
#define ATHR_VIRT_CABLE_TEST_CONTROL     0x16
#define ATHR_LED_CONTROL                 0x18
#define ATHR_MANUAL_LED_OVERRIDE         0x19
#define ATHR_VIRT_CABLE_TEST_STATUS      0x1c
#define ATHR_DEBUG_PORT1_ADDRESS_OFFSET  0x1d
#define ATHR_DEBUG_PORT2_DATA_PORT       0x1e
#endif /* !ATHR_PHY_CONTROL */

#define ATHR_PHY_ID1_EXPECTED            0x004d
#define ATHR_AR8012_PHY_ID2_EXPECTED     0xd021
#define ATHR_AR8216_PHY_ID2_EXPECTED     0xd014
#define ATHR_AR831x_PHY_ID2_EXPECTED     0xd041
#define ATHR_AR8021_PHY_ID2_EXPECTED     0xd04e
#define ATHR_AR8035_PHY_ID2_EXPECTED     0xd072

/* ATHR_PHY_CONTROL fields */
#define ATHR_CTRL_SOFTWARE_RESET                    0x8000
#define ATHR_CTRL_SPEED_LSB                         0x2000
#define ATHR_CTRL_AUTONEGOTIATION_ENABLE            0x1000
#define ATHR_CTRL_RESTART_AUTONEGOTIATION           0x0200
#define ATHR_CTRL_SPEED_FULL_DUPLEX                 0x0100
#define ATHR_CTRL_SPEED_MSB                         0x0040

#define ATHR_RESET_DONE(phy_control)                   \
    (((phy_control) & (ATHR_CTRL_SOFTWARE_RESET)) == 0)

/* Phy status fields */
#define ATHR_STATUS_AUTO_NEG_DONE                   0x0020

#define ATHR_AUTONEG_DONE(ip_phy_status)                   \
    (((ip_phy_status) &                                  \
        (ATHR_STATUS_AUTO_NEG_DONE)) ==                    \
        (ATHR_STATUS_AUTO_NEG_DONE))

/* Link Partner ability */
#define ATHR_LINK_100BASETX_FULL_DUPLEX       0x0100
#define ATHR_LINK_100BASETX                   0x0080
#define ATHR_LINK_10BASETX_FULL_DUPLEX        0x0040
#define ATHR_LINK_10BASETX                    0x0020

/* Advertisement register. */
#define ATHR_ADVERTISE_NEXT_PAGE              0x8000
#define ATHR_ADVERTISE_ASYM_PAUSE             0x0800
#define ATHR_ADVERTISE_PAUSE                  0x0400
#define ATHR_ADVERTISE_100FULL                0x0100
#define ATHR_ADVERTISE_100HALF                0x0080
#define ATHR_ADVERTISE_10FULL                 0x0040
#define ATHR_ADVERTISE_10HALF                 0x0020

#define ATHR_ADVERTISE_ALL (ATHR_ADVERTISE_10HALF | ATHR_ADVERTISE_10FULL | \
			ATHR_ADVERTISE_100HALF | ATHR_ADVERTISE_100FULL | \
			ATHR_ADVERTISE_ASYM_PAUSE | ATHR_ADVERTISE_PAUSE)

#define ATHR_ADVERTISE_10M (ATHR_ADVERTISE_10HALF | ATHR_ADVERTISE_10FULL | \
			ATHR_ADVERTISE_ASYM_PAUSE | ATHR_ADVERTISE_PAUSE)

/* 1000BASET_CONTROL */
#define ATHR_ADVERTISE_1000FULL               0x0200

/* Phy Specific status fields */
#define ATHER_STATUS_LINK_MASK                0xC000
#define ATHER_STATUS_LINK_SHIFT               14
#define ATHER_STATUS_FULL_DEPLEX              0x2000
#define ATHR_STATUS_LINK_PASS                 0x0400
#define ATHR_STATUS_RESOLVED                  0x0800

#ifndef BOOL
#define BOOL    int
#endif

#ifdef CONFIG_AR7242_8035_PHY
#undef HEADER_REG_CONF
#undef HEADER_EN
#endif

int athr8035_phy_is_up(int macUnit);
int athr8035_phy_is_fdx(int macUnit);
int athr8035_phy_speed(int macUnit);
u32 athr8035_phy_get_xmii_config(int macUnit, u32 speed);
BOOL athr8035_probe(int macUnit);
BOOL athr8035_phy_setup(int macUnit);

#endif /* _ATHR8035_PHY_H */

