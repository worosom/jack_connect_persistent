#
# Simple Makefile
#


#
# Configuration
#

CC=g++
CFLAGS=-Wall -std=c++11
LDFLAGS=-ljack


#
# Rules
#

.PHONY: all clean
all: client

clean:
	$(RM) jack_persistent_client.o jack_persistent_client

client:
	$(CC) $(CFLAGS) connect_persistent.cpp -o jack_persistent_client $(LDFLAGS)
