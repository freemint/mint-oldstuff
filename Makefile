# Makefile for virtual consoles and friends; nothing special...

CC = gcc # edit asm{}s or make them external .s if you try another one...

# experimental light/italic support... might not work on console
# (unless -DVT00XCON), won't yet work with char cells != 8x8 or 8x16
DVLIGHT = -DV_LIGHT

# debug:
#CFLAGS = -g -mshort -O2 -Wall
#LFLAGS = -g -mshort
CFLAGS = -mshort -mpcrel -O2 -Wall -fomit-frame-pointer $(DVLIGHT)
LFLAGS = -mshort
LIBS = -liio

# MiNT 1.11 version
M111 = -DWRITEB111

# default font search path for ttyvfont
DEFFONTPATH = -DDEFFONTPATH=\"/usr/share/misc/ttyvfonts:.\"

# ctrl-alt-num-( execl args
TGETTY = -DTGETTYUTMP '-DTGETTY="/bin/runtt", "runtt", "-t", tty, "/bin/nice", "-20", "/usr/etc/getty"'
#TGETTY = -DTGETTYUTMP '-DTGETTY="/bin/runtt", "runtt", "-t", tty, "/bin/nice", "-20", "/etc/mgetty", "-rb", "-p", prompt, tty'

# files...
SRC = Makefile README execgem.c runtt.c select0.c 1.12-filesys.h-diffs \
	 filesys.h vcon.h vtdev.h daemon.c paint.c screen.c vtdev.c ttyvfont.c
# default executables
DEFAULTX = execgem execmtos runtt select0.TOS ttyvfont vconsd save_res.tos
# other executables (see README)
MOREX = vcons1d vconx vcon

all: $(DEFAULTX)
	@echo done.

everything: $(DEFAULTX) $(MOREX)
	@echo done.

clean:
	rm -f *.o *.sym $(DEFAULTX) $(MOREX)

ttyvdev.tar.gz: $(SRC) ttyvfonts
	tar cfvz ttyvdev.tar.gz $(SRC) ttyvfonts

tar: ttyvdev.tar.gz

execgem: execgem.c
	$(CC) $(CFLAGS) $< -o execgem $(LIBS)

execmtos: execgem.c
	$(CC) $(CFLAGS) -DINITPRG=\"/usr/multitos/gem.sys\" $< -o execmtos $(LIBS)

runtt: runtt.c
	$(CC) $(CFLAGS) $< -o runtt $(LIBS)

select0.TOS: select0.c
	$(CC) $(CFLAGS) $< -o select0.TOS

ttyvfont: ttyvfont.c
	$(CC) $(CFLAGS) $(DEFFONTPATH) $< -o ttyvfont $(LIBS)

vconsd: daemon.o vtdevxd.o paintx.o screen.o
	$(CC) -G $(LFLAGS) daemon.o vtdevxd.o paintx.o screen.o -ovconsd
	toglclr --super $@

save_res.tos: save_res.c falconres.h
	$(CC) save_res.c -osave_res.tos

vcons1d: daemon.o vtdevx1.o screen1.o
	$(CC) -G $(LFLAGS) daemon.o vtdevx1.o screen1.o -ovcons1d
	toglclr --super $@

vconx: daemon.o vtdevx.o paintx.o
	$(CC) -G $(LFLAGS) daemon.o vtdevx.o paintx.o -ovconx
	toglclr --super $@

vcon: daemon.o vtdev.o paint.o
	$(CC) -G $(LFLAGS) daemon.o vtdev.o paint.o -ovcon
	toglclr --super $@

vcon.sym: daemon.o vtdev.o
	$(CC) -B/usr/lib/sym- $(LFLAGS) daemon.o vtdev.o -ovcon.sym

daemon.o: daemon.c
	$(CC) $(CFLAGS) $(TGETTY) -c $< -o daemon.o

paint.o: paint.c
	$(CC) $(CFLAGS) -funroll-loops -c $< -o paint.o

vtdev.o: vtdev.c
	$(CC) $(CFLAGS) $(M111) -c $< -o vtdev.o

vtdevx.o: vtdev.c
	$(CC) $(CFLAGS) -DVT00XCON $(M111) -c $< -o vtdevx.o

screen.o: screen.c falconres.h
	$(CC) $(CFLAGS) -DVMODE -c $< -o screen.o

vtdevxd.o: vtdev.c
	$(CC) $(CFLAGS) -DVT00XCON -DVMODE $(M111) -c $< -o vtdevxd.o

paintx.o: paint.c
	$(CC) $(CFLAGS) -funroll-loops -DVT00XCON -c $< -o paintx.o

screen1.o: screen.c
	$(CC) $(CFLAGS) -DFORCE1PLANE -DVMODE -c $< -o screen1.o

vtdevx1.o: vtdev.c paint.c
	$(CC) $(CFLAGS) -DFORCE1PLANE -DVMODE $(M111) -c $< -o vtdevx1.o
