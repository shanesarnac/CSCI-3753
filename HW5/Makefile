# File: Makefile
# By: Andy Sayler <www.andysayler.com>
# Adopted from work by: Chris Wailes <chris.wailes@gmail.com>
# Project: CSCI 3753 Programming Assignment 5
# Creation Date: 2010/04/06
# Modififed Date: 2012/04/12
# Description:
#	This is the Makefile for PA5.


CC           = gcc

CFLAGSFUSE   = `pkg-config fuse --cflags`
LLIBSFUSE    = `pkg-config fuse --libs`
LLIBSOPENSSL = -lcrypto

CFLAGS = -c -g -Wall -Wextra
LFLAGS = -g -Wall -Wextra
 
PA5_ENCFS = pa5-encfs

.PHONY: all fuse-examples xattr-examples openssl-examples clean

all: pa5-encfs

pa5-encfs: pa5-encfs.o
	$(CC) $(CLAGS) $^ -o $@ $(LLIBSFUSE)

pa5-encfs.o: pa5-encfs.c
	$(CC) $(CFLAGS) $(CFLAGSFUSE) $<

clean:
	rm -f *.o
	rm -f *~



