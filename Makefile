# Makefile for virtual consoles and friends; nothing special...

CC = gcc # edit asm{}s or make them external .s if you try another one...

# debug:
#CFLAGS = -g -mshort -O2 -Wall
#LFLAGS = -g -mshort
CFLAGS = -mshort -mpcrel -O2 -Wall -fomit-frame-pointer
LFLAGS = -mshort
LIBS = -liio16

all: execgem execmtos runtt select0.TOS vconsd

execgem: execgem.c
	$(CC) $(CFLAGS) $< -o execgem $(LIBS)

execmtos: execgem.c
	$(CC) $(CFLAGS) -DINITPRG=\"gem.sys\" $< -o execmtos $(LIBS)

runtt: runtt.c
	$(CC) $(CFLAGS) $< -o runtt $(LIBS)

select0.TOS: select0.c
	$(CC) $(CFLAGS) $< -o select0.TOS

vconsd: daemon.o vtdevxd.o paintx.o screen.o
	$(CC) -G $(LFLAGS) daemon.o vtdevxd.o paintx.o screen.o -ovconsd
	toglclr -super $@
	@echo done.

vcons1d: daemon.o vtdevx1.o screen1.o
	$(CC) -G $(LFLAGS) daemon.o vtdevx1.o screen1.o -ovcons1d
	toglclr -super $@
	@echo done.

vconx: daemon.o vtdevx.o paintx.o
	$(CC) -G $(LFLAGS) daemon.o vtdevx.o paintx.o -ovconx
	toglclr -super $@
	@echo done.

vcon: daemon.o vtdev.o paint.o
	$(CC) -G $(LFLAGS) daemon.o vtdev.o paint.o -ovcon
	toglclr -super $@
	@echo done.

vcon.sym: daemon.o vtdev.o
	$(CC) -B/usr/lib/sym- $(LFLAGS) daemon.o vtdev.o -ovcon.sym
	@echo done.

paint.o: paint.c
	$(CC) $(CFLAGS) -funroll-loops -c $< -o paint.o

vtdevx.o: vtdev.c
	$(CC) $(CFLAGS) -DVT00XCON -c $< -o vtdevx.o

screen.o: screen.c
	$(CC) $(CFLAGS) -DVMODE -c $< -o screen.o

vtdevxd.o: vtdev.c
	$(CC) $(CFLAGS) -DVT00XCON -DVMODE -c $< -o vtdevxd.o

paintx.o: paint.c
	$(CC) $(CFLAGS) -funroll-loops -DVT00XCON -c $< -o paintx.o

screen1.o: screen.c
	$(CC) $(CFLAGS) -DFORCE1PLANE -DVMODE -c $< -o screen1.o

vtdevx1.o: vtdev.c paint.c
	$(CC) $(CFLAGS) -DFORCE1PLANE -DVMODE -c $< -o vtdevx1.o
