#!/bin/csh
set history=30
set autolist
if ( $user == root ) then
   set prompt='# '
else
   set prompt='% '
endif
set prompt2='? '

set path=( /bin /usr/local/bin /usr/bin /usr/ucb /gnu/bin /usr/games . )

set ignoreeof=1
set notify=1

rehash

#
# environment: this is for my setup, customize as you see fit
#

# Sometime, there are lots of places that one can find man pages.
setenv MANPATH /usr/local/man:/usr/man
setenv TEXINPUTS .:$HOME/bin/tex:$HOME/bin:/usr/lib/tex/macros

# Where's the Gnu stuff at?
setenv BISON_SIMPLE /usr/lib/bison.sim
setenv BISON_HAIRY /usr/lib/bison.hai

setenv GCCEXEC /gnu/gnuc/gcc-
setenv GNULIB /usr/lib
setenv GNUINC /usr/include

setenv TMP /tmp
setenv TEMP /tmp
setenv TMPDIR /tmp

setenv LSOPTS C

#setenv TERMCAP /etc/termcap

setenv TZ GMT
setenv UNIXMODE "/cub"
setenv SHELL /bin/sh

#
# aliases:
#

#alias ls ls-F
alias pwd echo \$cwd
alias j jobs -l
alias po popd
alias pu pushd
alias edit jove
