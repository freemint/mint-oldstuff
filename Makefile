# Makefile for vcon; nothing special...

CC = gcc
# debug:
#CFLAGS = -g -mshort -O2 -Wall
#LFLAGS = -g -mshort
CFLAGS = -mshort -O2 -Wall -fomit-frame-pointer
LFLAGS = -mshort -fomit-frame-pointer

vcon: vcon.o vtdev.o
	$(CC) -G $(LFLAGS) vcon.o vtdev.o -ovcon
	@echo done.

vcon.sym: vcon.o vtdev.o
	$(CC) -B/usr/lib/sym- $(LFLAGS) vcon.o vtdev.o -ovcon.sym
	@echo done.

vconx: vcon.o vtdevx.o
	$(CC) -G $(LFLAGS) vcon.o vtdevx.o -ovconx
	@echo done.

vtdevx.o: vtdev.c
	$(CC) $(CFLAGS) -DVT00XCON -c $< -o vtdevx.o
