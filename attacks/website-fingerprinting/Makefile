all: flush-monitor

CC := gcc
CFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter

GLOBAL_INCLUDE_DIR := ../../utilities/include
GLOBAL_INCLUDES := $(shell find $(GLOBAL_INCLUDE_DIR)/ -name '*.h')

GLOBAL_UTILS_DIR := ../../utilities
GLOBAL_UTILS := $(shell find $(GLOBAL_UTILS_DIR)/ -name '*.c')

################################################################################
flush-monitor: flush-monitor.c $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(CC) flush-monitor.c                                                      \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS)                                                              \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o flush-monitor

.PHONY: clean 

clean:
	rm -f flush-monitor
