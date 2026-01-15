all: watcher

##################################
########## CONFIGURE ME ##########
READ_THRESHOLD := 0.000030000
##################################

CC := gcc
CFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter

watcher: watcher.c
	$(CC) watcher.c $(CFLAGS) -DREAD_THRESHOLD=$(READ_THRESHOLD) -o watcher

clean:
	rm watcher