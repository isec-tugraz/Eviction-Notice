all: flush read-bypass-backward-reading read-bypass-readahead

CC := gcc
CFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter

BUILD_DIR := ./build
builddir := @mkdir -p $(BUILD_DIR)

INCLUDE_DIR := ../utilities/include
INCLUDES := $(shell find $(INCLUDE_DIR)/ -name '*.h')

UTILS_DIR := ../utilities/
UTILS := $(shell find $(UTILS_DIR)/ -name '*.c')

flush: flush.c $(INCLUDES) $(UTILS)
	$(builddir)
	$(CC) flush.c                            \
		$(UTILS)                             \
		$(CFLAGS)                            \
		-I$(INCLUDE_DIR)                     \
		-o $(BUILD_DIR)/calibrate-flush

read-bypass-backward-reading: read-bypass-backward-reading.c $(INCLUDES) $(UTILS)
	$(builddir)
	$(CC) read-bypass-backward-reading.c             \
		$(UTILS)                                     \
		$(CFLAGS)                                    \
		-I$(INCLUDE_DIR)                             \
		-o $(BUILD_DIR)/read-bypass-backward-reading

read-bypass-readahead: read-bypass-readahead.c $(INCLUDES) $(UTILS)
	$(builddir)
	$(CC) read-bypass-readahead.c             \
		$(UTILS)                                     \
		$(CFLAGS)                                    \
		-I$(INCLUDE_DIR)                             \
		-o $(BUILD_DIR)/read-bypass-readahead

.PHONY: clean 

clean:
	rm -f $(BUILD_DIR)/* $(INCLUDE_DIR)/*.h.gch
