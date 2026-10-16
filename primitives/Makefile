all: flush monitor reload evict

##################################
########## CONFIGURE ME ##########
READ_THRESHOLD := 0.000030000
READAHEAD_THRESHOLD := 0.000002500
FLUSH_THRESHOLD := 0.000000900
##################################

CC := gcc
CFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter

BUILD_DIR := ./build
builddir := @mkdir -p $(BUILD_DIR)

LOC_INCLUDE_DIR := ./include
LOC_INCLUDES := $(shell find $(LOC_INCLUDE_DIR)/ -name '*.h')

LOC_UTILS_DIR := ./utils
LOC_UTILS := $(shell find $(LOC_UTILS_DIR)/ -name '*.c')

GLOBAL_INCLUDE_DIR := ../utilities/include
GLOBAL_INCLUDES := $(shell find $(GLOBAL_INCLUDE_DIR)/ -name '*.h')

GLOBAL_UTILS_DIR := ../utilities
GLOBAL_UTILS := $(shell find $(GLOBAL_UTILS_DIR)/ -name '*.c')

################################################################################
flush: flush/flush.c $(LOC_INCLUDES) $(LOC_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) flush/flush.c                                                        \
		$(LOC_UTILS)                                                           \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS)                                                              \
		-DFLUSH_THRESHOLD=$(FLUSH_THRESHOLD)                                   \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o $(BUILD_DIR)/flush

################################################################################
monitor-cachestat: monitor/cachestat.c monitor/monitor.c $(LOC_INCLUDES) $(LOC_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) -DMON_CACHESTAT                                                      \
		monitor/cachestat.c                                                    \
		monitor/monitor.c                                                      \
		$(LOC_UTILS)                                                           \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS)                                                              \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o $(BUILD_DIR)/monitor-cachestat

monitor-preadv2: monitor/preadv2.c monitor/monitor.c $(LOC_INCLUDES) $(LOC_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) -DMON_PREADV2                                                        \
		monitor/preadv2.c                                                      \
		monitor/monitor.c                                                      \
		$(LOC_UTILS)                                                           \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS)                                                              \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o $(BUILD_DIR)/monitor-preadv2

monitor: monitor-cachestat monitor-preadv2

################################################################################
read: reload/read.c $(LOC_INCLUDES) $(LOC_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) reload/read.c                                                        \
		$(LOC_UTILS)                                                           \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS)                                                              \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o $(BUILD_DIR)/read

read-bypass-readahead: reload/read-bypass-readahead.c $(LOC_INCLUDES) $(LOC_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) reload/read-bypass-readahead.c                                       \
		$(LOC_UTILS)                                                           \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS)                                                              \
		-DREADAHEAD_THRESHOLD=$(READAHEAD_THRESHOLD)                           \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o $(BUILD_DIR)/read-bypass-readahead

read-bypass-backward-reading: reload/read-bypass-backward-reading.c $(LOC_INCLUDES) $(LOC_GLOBAL) $(LOC_GLOBAL) $(LOC_UTILS)
	$(builddir)
	$(CC) reload/read-bypass-backward-reading.c                                \
		$(LOC_UTILS)                                                           \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS)                                                              \
		-DREAD_THRESHOLD=$(READ_THRESHOLD)                                     \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o $(BUILD_DIR)/read-bypass-backward-reading

reload: read read-bypass-readahead read-bypass-backward-reading

################################################################################
evict-1-shlibs: evict/evict-1-shlibs.c $(LOC_INCLUDES) $(LOC_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) evict/evict-1-shlibs.c                                               \
		$(LOC_UTILS)                                                           \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS)                                                              \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o $(BUILD_DIR)/evict-1-shlibs

evict-2-randaccess: evict/evict-2-randaccess.c $(LOC_INCLUDES) $(LOC_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) evict/evict-2-randaccess.c                                           \
		$(LOC_UTILS)                                                           \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS)                                                              \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o $(BUILD_DIR)/evict-2-randaccess

evict-3-baseline: evict/evict-3-baseline.c $(LOC_INCLUDES) $(LOC_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) evict/evict-3-baseline.c                                             \
		$(LOC_UTILS)                                                           \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS)                                                              \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o $(BUILD_DIR)/evict-3-baseline

evict-4-dynpressure: evict/evict-4-dynpressure.c $(LOC_INCLUDES) $(LOC_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) evict/evict-4-dynpressure.c                                          \
		$(LOC_UTILS)                                                           \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS) -pthread                                                     \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o $(BUILD_DIR)/evict-4-dynpressure

evict: evict-1-shlibs evict-2-randaccess evict-3-baseline evict-4-dynpressure

.PHONY: clean 

clean:
	rm -f $(BUILD_DIR)/* $(LOC_INCLUDE_DIR)/*.h.gch
