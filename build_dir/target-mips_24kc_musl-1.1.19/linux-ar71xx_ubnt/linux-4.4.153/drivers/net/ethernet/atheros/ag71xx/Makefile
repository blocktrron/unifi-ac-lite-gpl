#
# Makefile for the Atheros AR71xx built-in ethernet macs
#

ag71xx-y	+= ag71xx_main.o
ag71xx-y	+= ag71xx_ethtool.o
ag71xx-y	+= ag71xx_phy.o
ag71xx-y	+= ag71xx_mdio.o
ag71xx-y	+= ag71xx_ar7240.o

ccflags-$(CONFIG_AG71XX_INCREASE_RX_TX_RING_SIZE) += -DAG71XX_TX_RING_SIZE_DEFAULT=3072
ccflags-$(CONFIG_AG71XX_INCREASE_RX_TX_RING_SIZE) += -DAG71XX_RX_RING_SIZE_DEFAULT=2048
ccflags-$(CONFIG_AG71XX_INCREASE_RX_TX_RING_SIZE) += -DAG71XX_TX_RING_SIZE_MAX=3072
ccflags-$(CONFIG_AG71XX_INCREASE_RX_TX_RING_SIZE) += -DAG71XX_RX_RING_SIZE_MAX=2048

ag71xx-$(CONFIG_AG71XX_DEBUG_FS)	+= ag71xx_debugfs.o
ag71xx-$(CONFIG_AG71XX_AR8216_SUPPORT)	+= ag71xx_ar8216.o

obj-$(CONFIG_AG71XX)	+= ag71xx.o

