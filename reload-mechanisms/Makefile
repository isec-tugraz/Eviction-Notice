CC := gcc
CFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter

BUILD_DIR := ./build
builddir := @mkdir -p $(BUILD_DIR)

SRCS = $(wildcard *.c)
OUTPUTS = $(SRCS:.c=)

all: $(OUTPUTS)

LOC_INCLUDE_DIR := ./include
LOC_INCLUDES := $(shell find $(LOC_INCLUDE_DIR)/ -name '*.h')

LOC_UTILS_DIR := ./utils
LOC_UTILS := $(shell find $(LOC_UTILS_DIR)/ -name '*.c')

GLOBAL_INCLUDE_DIR := ../utilities/include
GLOBAL_INCLUDES := $(shell find $(GLOBAL_INCLUDE_DIR)/ -name '*.h')

GLOBAL_UTILS_DIR := ../utilities
GLOBAL_UTILS := $(shell find $(GLOBAL_UTILS_DIR)/ -name '*.c')

%: %.c $(LOC_INCLUDES) $(LOC_UTILS) $(GLOBAL_INCLUDES) $(GLOBAL_UTILS)
	$(builddir)
	$(CC) $<                                                                   \
		$(LOC_UTILS)                                                           \
		$(GLOBAL_UTILS)                                                        \
		$(CFLAGS)                                                              \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-I$(GLOBAL_INCLUDE_DIR)                                                \
		-o $(BUILD_DIR)/$@