# Makefile for vcon; nothing special...

CC = gcc
CFLAGS = -g -mshort -O2 -Wall
LDFLAGS = -g -mshort

vcon: vcon.o vtdev.o
	$(CC) -G $(LDFLAGS) vcon.o vtdev.o -ovcon
	@echo done.

vcon.sym: vcon.o vtdev.o
	$(CC) -B/usr/lib/sym- $(LDFLAGS) vcon.o vtdev.o -ovcon.sym
	@echo done.

