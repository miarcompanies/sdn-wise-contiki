CONTIKI_PROJECT = sdn-wise
all: $(CONTIKI_PROJECT)

SMALL = 1
CONTIKI = ../contiki
CONTIKI_WITH_RIME = 1
#CONTIKI_WITH_IPV6 = 1
#CONTIKI_WITH_RPL = 0

PROJECT_SOURCEFILES = flowtable.c packet-buffer.c address.c neighbor-table.c packet-handler.c node-conf.c packet-creator.c
TARGETDIRS += ../targets
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
#Million added for optimization
#CFLAGS += -ffunction-sections
# LDFLAGS += -Wl,--gc-sections,--undefined=_reset_vector__,--undefined=InterruptVectors,--undefined=_copy_data_init__,--undefined=_clear_bss_init__,--undefined=_end_of_init__
include $(CONTIKI)/Makefile.include
