#
# Copyright (c) 2013 Qualcomm Atheros, Inc.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

# ROM version
ifdef BOOT_FROM_NAND #{
ifeq ($(COMPRESSED_UBOOT),1) #{
TEXT_BASE = 0x80010000
BOOTSTRAP_TEXT_BASE = 0xa0100000
else #}{
TEXT_BASE = 0xa0100000
endif #}
else #}{
ifeq ($(COMPRESSED_UBOOT),1) #{
TEXT_BASE = 0x80010000
BOOTSTRAP_TEXT_BASE = 0x9f000000
else #}{
TEXT_BASE = 0x9f000000
endif #}
endif #}
# TEXT_BASE = 0xbf000000

# SDRAM version
# TEXT_BASE = 0x80000000

# RAM version
# TEXT_BASE = 0x83fc0000
# TEXT_BASE = 0x80100000

export BOARD_EXTRA_OBJS_tmp =

BOARD_EXTRA_OBJS_tmp += $(TOPDIR)/board/$(BOARDDIR)/extra.o
ifeq ($(BOOT_FROM_NAND),1)
BOARD_EXTRA_OBJS_tmp += $(TOPDIR)/board/$(VENDOR)/common/ath_nand.o $(TOPDIR)/drivers/nand/nand_ids.o
endif

ifeq ($(COMPRESSED_UBOOT),1) #{
BOARD_EXTRA_OBJS = $(BOARD_EXTRA_OBJS_tmp)
endif

UBNT_APP=1
ETH_CONFIG2=_s17
ATH_SGMII_FORCED=1
ATH_S17_PHY0_WAN=1
ATH_RGMII_CAL=1

export UBNT_APP
export ETH_CONFIG2
export ATH_SGMII_FORCED
export ATH_S17_PHY0_WAN
export ATH_RGMII_CAL
