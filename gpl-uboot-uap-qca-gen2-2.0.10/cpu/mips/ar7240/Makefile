include $(TOPDIR)/config.mk

LIB	= lib$(SOC).a

START	=
#OBJS	= ar7240_serial.o ag7240.o ar7240_flash.o meminit.o

OBJS	= meminit.o 

ifeq ($(BOARD), $(filter $(BOARD),ap121 uap-hornet uap-qca933x))
OBJS	+= hornet_serial.o
SOBJS	+= hornet_ddr_init.o
else
OBJS	+= ar7240_serial.o
endif

ifneq ($(BUILD_EMU), )
OBJS    += hornet_ag7240.o
else
ifeq ($(BOARD), db12x)
OBJS    += ag934x.o
else
ifeq ($(BOARD),ap123)
OBJS    += ag934x.o
else
ifeq ($(BOARD), uap-ar9341)
OBJS    += ag934x.o
else
ifeq ($(BOARD), uap-ar9344)
OBJS    += ag934x.o
else
ifeq ($(BOARD), uap-qca9342)
OBJS    += ag934x.o
else
ifeq ($(BOARD), ubnt-R5X3)
OBJS    += ag934x.o
else
OBJS    += ag7240.o
endif
endif
endif
endif
endif
endif
endif

SOBJS	+= 

all:	.depend $(START) $(LIB)

$(LIB):	$(OBJS) $(SOBJS) $(OBJS-0)
	$(AR) crv $@ $(OBJS) $(SOBJS)

#########################################################################

.depend:	Makefile $(START:.o=.S) $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(START:.o=.S) $(OBJS:.o=.c) > $@

sinclude .depend

#########################################################################
