include $(TOPDIR)/config.mk

LIB	= lib$(BOARD).a

OBJS	= $(BOARD).o flash.o   ../common/ddr/ddr_dt.o

ifeq ($(BOARD_DDR_TYPE),DDR1)
	OBJS += ../common/ddr/ddr1_init.o

else
ifeq ($(BOARD_DDR_TYPE),DDR2)
	OBJS += ../common/ddr/ddr2_init.o

else
ifeq ($(BOARD_DDR_TYPE),DDR3)
	OBJS += ../common/ddr/ddr3_init.o

else
ifeq ($(BOARD_DDR_TYPE),SDRAM)
	OBJS += ../common/ddr/sdram_init.o

else
    $(error pls specify BOARD_DDR_TYPE{DDR1|DDR2|DDR3|SDRAM})

endif #$(SDRAM)
endif #$(DDR3)
endif #$(DDR2)
endif #$(DDR1)

ifdef BOOT_FROM_NAND
	OBJS    +=  music_nand_flash.o
else
    OBJS    +=  ../common/music_spi_flash.o
	#OBJS    +=  music_spi_flash.o
endif

OBJS    +=  ../common/flash_oem.o

$(LIB):	.depend $(OBJS) $(SOBJS)
	$(AR) crv $@ $(OBJS) $(SOBJS)

#########################################################################

.depend:	Makefile $(SOBJS:.o=.S) $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(SOBJS:.o=.S) $(OBJS:.o=.c) > $@

sinclude .depend

#########################################################################
