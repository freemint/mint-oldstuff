#	%W% (Berkeley) %G% 
PROG=sh
STUBS=builtins cd dirent echo error eval exec expand input jobs mail \
  main memalloc miscblti mystring nodes options parser redir show signames \
  syntax trap output var init
OBJS=$(addsuffix .o, $(STUBS))
SRCS=$(addsuffix .c, $(STUBS))
HEADERS=bltin.h builtins.h errmsg.h error.h eval.h exec.h expand.h init.h \
  input.h jobs.h machdep.h mail.h main.h memalloc.h mystring.h nodes.h \
  options.h output.h parser.h redir.h shell.h signames.h \
  syntax.h trap.h var.h
CFLAGS=-DSHELL -I. $(ADDITIONALCFLAGS)
LDFLAGS=
LOADLIBES=
FUNCS=$(addprefix funcs/, cmv dirs kill login newgrp popd pushd suspend)
DOCS=dump dinfo lastdump sh.1 tour test.1 readme.st copying copying.sh
ORGFILES=makefile.org token.def mktokens mkbuilti builtins.def mkinit.c \
  mknodes.c nodetype nodes_c.pat mksignam.c mksyntax.c
SDISTFILES=$(SRCS) $(HEADERS) $(FUNCS) $(DOCS) $(ORGFILES) makefile test.c
BDISTFILES=$(FUNCS) $(DOCS) sh.ttp test.ttp

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LOADLIBES)
	../../bin/ttramify $(PROG)

sh-src.zoo: $(SDISTFILES)
	rm -f sh-src.zoo
	zoo ah sh-src.zoo $(SDISTFILES)

sh-bin.zoo: $(BDISTFILES)
	rm -f sh-bin.zoo
	zoo ah sh-bin.zoo $(BDISTFILES)

test: test.o
	$(CC) $(LDFLAGS) -o test test.o $(LOADLIBES)

builtins.o: shell.h builtins.h 
cd.o: shell.h var.h nodes.h jobs.h options.h output.h memalloc.h error.h \
  mystring.h 
dirent.o: shell.h
echo.o: bltin.h
errmsg.o: shell.h output.h errmsg.h
error.o: shell.h main.h options.h output.h error.h 
eval.o: shell.h nodes.h syntax.h expand.h parser.h jobs.h eval.h builtins.h \
  options.h exec.h redir.h input.h output.h trap.h var.h memalloc.h error.h \
  mystring.h 
exec.o: shell.h main.h nodes.h parser.h redir.h eval.h exec.h builtins.h \
  var.h options.h input.h output.h syntax.h memalloc.h error.h init.h \
  mystring.h 
expand.o: shell.h main.h nodes.h eval.h expand.h syntax.h parser.h jobs.h \
  options.h var.h input.h output.h memalloc.h error.h mystring.h 
init.o: shell.h mystring.h eval.h input.h error.h options.h redir.h \
  signames.h trap.h output.h memalloc.h var.h 
input.o: syntax.h input.h output.h memalloc.h error.h 
jobs.o: shell.h main.h parser.h nodes.h jobs.h options.h trap.h \
  signames.h syntax.h input.h output.h memalloc.h error.h mystring.h 
mail.o: shell.h exec.h var.h output.h memalloc.h error.h 
main.o: shell.h main.h mail.h options.h output.h parser.h nodes.h \
  eval.h jobs.h input.h trap.h var.h memalloc.h error.h init.h mystring.h 
memalloc.o: shell.h output.h memalloc.h error.h machdep.h mystring.h 
miscblti.o: shell.h options.h var.h output.h memalloc.h error.h mystring.h 
mystring.o: shell.h syntax.h error.h mystring.h 
nodes.o: shell.h nodes.h memalloc.h machdep.h mystring.h 
options.o: shell.h options.h nodes.h eval.h jobs.h input.h output.h trap.h \
  var.h memalloc.h error.h mystring.h 
output.o: shell.h syntax.h output.h memalloc.h error.h
parser.o: shell.h parser.h nodes.h expand.h redir.h syntax.h options.h \
  input.h output.h var.h error.h memalloc.h mystring.h 
redir.o: shell.h nodes.h jobs.h expand.h redir.h output.h memalloc.h error.h 
show.o: shell.h parser.h nodes.h mystring.h
signames.o: shell.h
syntax.o: shell.h syntax.h
trap.o: shell.h main.h nodes.h eval.h jobs.h options.h syntax.h signames.h \
  output.h memalloc.h error.h trap.h mystring.h
var.o: shell.h output.h expand.h nodes.h eval.h exec.h syntax.h options.h \
  mail.h var.h memalloc.h error.h mystring.h  

clean:
	/bin/rm sh test *.o

install: sh test
	cp sh $(USRBINDIR)/sh$(TTPEXT)
	chmod 0755 $(USRBINDIR)/sh$(TTPEXT)
	cp test $(USRBINDIR)/test$(TTPEXT)
	chmod 0755 $(USRBINDIR)/test$(TTPEXT)
	cp test $(USRBINDIR)/[$(TTPEXT)
	chmod 0755 $(USRBINDIR)/[$(TTPEXT)
