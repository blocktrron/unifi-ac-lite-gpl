include ../Config

ifeq ($(IP_CONFIG_SETNS),y)
	CFLAGS += -DHAVE_SETNS
endif

ifeq ($(IP_CONFIG_TINY),y)
  CFLAGS += -DIPROUTE2_TINY
endif

CFLAGS += $(FPIC)

UTILOBJ = utils.o rt_names.o ll_types.o ll_proto.o ll_addr.o \
	inet_proto.o namespace.o json_writer.o \
	names.o color.o

NLOBJ=libgenl.o ll_map.o libnetlink.o

all: libnetlink.a libutil.a

libnetlink.a: $(NLOBJ)
	$(AR) rcs $@ $(NLOBJ)

libutil.a: $(UTILOBJ) $(ADDLIB)
	$(AR) rcs $@ $(UTILOBJ) $(ADDLIB)

install:

clean:
	rm -f $(NLOBJ) $(UTILOBJ) $(ADDLIB) libnetlink.a libutil.a

