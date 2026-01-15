all: receiver-preadv2 receiver-cachestat sender

CC := gcc
CFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-discarded-qualifiers

BUILD_DIR := ./build-cc-fm
builddir := @mkdir -p $(BUILD_DIR)

LOCAL_INCLUDE_DIR := ../utils/include
LOCAL_INCLUDES := $(shell find $(LOCAL_INCLUDE_DIR)/ -name '*.h')

LOCAL_UTILS_DIR := ../utils/
LOCAL_UTILS := $(shell find $(LOCAL_UTILS_DIR)/ -name '*.c')

GLOBAL_INCLUDE_DIR := ../../utilities/include
GLOBAL_INCLUDES := $(shell find $(GLOBAL_INCLUDE_DIR)/ -name '*.h')

GLOBAL_UTILS_DIR := ../../utilities
GLOBAL_UTILS := $(shell find $(GLOBAL_UTILS_DIR)/ -name '*.c')

RECV_OUTPUT := receiver

sender: sender.c $(LOCAL_INCLUDES) $(LOCAL_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) sender.c                          \
		$(GLOBAL_UTILS) $(LOCAL_UTILS)      \
		$(CFLAGS)                           \
		-I$(LOCAL_INCLUDE_DIR)              \
		-I$(GLOBAL_INCLUDE_DIR)             \
		-o $(BUILD_DIR)/sender

receiver-cachestat: receiver.c $(LOCAL_INCLUDES) $(LOCAL_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) receiver.c                        \
		$(GLOBAL_UTILS) $(LOCAL_UTILS)      \
		$(CFLAGS) -DUSE_CACHESTAT           \
		-I$(LOCAL_INCLUDE_DIR)              \
		-I$(GLOBAL_INCLUDE_DIR)             \
		-o $(BUILD_DIR)/receiver-cachestat

receiver-preadv2: receiver.c $(LOCAL_INCLUDES) $(LOCAL_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) receiver.c                        \
		$(GLOBAL_UTILS) $(LOCAL_UTILS)      \
		$(CFLAGS) -DUSE_PREADV2             \
		-I$(LOCAL_INCLUDE_DIR)              \
		-I$(GLOBAL_INCLUDE_DIR)             \
		-o $(BUILD_DIR)/receiver-preadv2

.PHONY: clean 

clean:
	rm -rf $(BUILD_DIR)
