Blurb for the May 1992 distribution

This is the May 1992 distribution of the /bin/sh-compatible shell by
Kenneth Almquist, called ash.  This is a port to the atari st for MiNT
0.93 (and possibly later versions, too.)  The original sources were
taken from the 4.4BSD sources.

Changes in the May 1992 distribution in a nutshell:

1. Added carriage-return stripping to backquote substitution
2. Corrected test.c
3. Added better dump scripts

Changes in detail:

1.  The backquote substitution did strip newlines, but nor carriage
returns.  Since the original author of ash used his own buffered-I/O
package, I could not easily use the stdio.h routines.  That's why the
bug struck.  It is fixed, and backquote substitution should not leave
extraneous carriage-returns in variables or files any more.

2.  I had mixed up the numeric and string operators, plus I named the
equality testing operator ==, not =. (*blush*).  This is fixed.  Please
forgive me.

3.  The dump scripts are much better, now that they have been used in
the recent level 0 dump of my hard disk.  The following programs are
included: 

	dump -- dump TOS partitions, either full or incremental
	usage: sh dump <level> <partition>...

	dinfo -- get information about a dump
	usage: sh dinfo <partition>...

	lastdump -- say when a file or set of files was last dumped
	usage: sh lastdump <regex>

Please look into the scripts before you use them; you will almost
certainly have to change some path names.  If you don't have gawk, sed,
or tar, get them from the excellent atari.archive.umich.edu.  If you
lack /bin/time, get the GNU shell utilities 1.6, and compile.  

[N.B: But *do not use %D or %Y as the format strings!* The bug that
strikes there is interesting.  Basically, what happens is:

	* GNU strftime() is POSIX, i.e., has %D and %Y.
	* Atari-gcc strftime() is ANSI, i.e., doesn't have %D and %Y.
	* strftime() returns 0 if *either* there is not enough room in
	  the string to put the formatted date, *or* no characters
	  could be written.  (This is such a bad design bug, that, IMHO,
	  the strftime() function is almost useless, because you cannot
	  distinguish between an overfull buffer and a bad format string.)
	* GNU date doesn't check for the correctness of the format
	  string passed to strftime().
	* When GNU date gets 0 from strftime(), it reallocs its buffer
	  by 200 bytes and tries again.
	* strftime (buf, "%D%Y", len) will return 0 for any len, if one
	  uses the Atari-gcc strftime().
	* You cannot easily use GNU strftime(), because the atari-gcc
	  strftime() refuses to be kicked out of the library.

Result: date fills up memory by repeatedly realloc()ing 200 more bytes,
but does not progress.  Takes quite some time on a 4MB atari st. :-)

So either use only ANSI-style format strings, or patch date to include
checking of the format string.]

Also, the dump information file, dumpinfo, gets quite large over the
time.  I have written a shell script that will purge dump information
selectively, e.g., for an old level 0 dump of /dev/c, but it's not
tested very well, so I'm waiting until I've fixed some loose ends. 


Future directions:

Making suffix stripping better; also, rewrite command hashing and
execution; the current code is cursed.  One person has reported bugs in
the signal handling code to me.  (Don't worry, they're only for stopping
processes that want to do output to a terminal and are currently in the
background; everything else appears to work OK.)  I'll look into that,
too. 

End of blurb for the May 1992 distribution


Original distribution: April 1992


0.  General Blurb
-----------------

This is a port of the Berkeley 4.4BSD version of /bin/sh for MiNT.  It
will probably run ONLY under MiNT; I haven't tested plain TOS.  Please
mail bug reports in the MiNT port to neuhaus@informatik.uni-kl.de.  You
can also write to me in paper mail.  The address is

Stephan Neuhaus
Hilgardring 32
D-6750 Kaiserslautern 
Germany

This port has been, well, `tested' is not the right word, but it's
close, on my 4MB 1040 STF with TOS 1.4, MiNT 0.93, MiNT Libraries
patchlevel 19, make version 3.60 (my own port to MiNT) and gcc version
1.40.  The makefile had to be hacked, because the original makefile used
some pretty heavy scripts to create some .c and .h files.  Those scripts
were written for, yeah, you guessed it, /bin/sh.  I ran them on a SUN,
took them to my atari, typed `make' and prayed.  No problems with these
ones.  And no problems with the libraries, either.  Thank you, Eric R. 
Smith and Jwahar R. Bammi! (And, of course all others who contributed
to the nice development system I have running!)

While we're at it, I would like to thank ATARI for shipping such a bad OS
that it inspired ERS to create MiNT.


1.  Things That Work and Things That Don't
------------------------------------------

I have written a quite extensive shell script to make dumps of MiNT
filesystems.  It is included in the file `dump'.  If you study this
script, and if you believe me that it works, you'll see that the
following features provably work (at least in my case):

  * Backquote substitution
  * Pipes
  * Traps (as in trap <command> signo...)
  * User-defined functions
  * I/O redirection
  * Variable substitution

However, if you actually run the script, you'll find strange things in
the logs afterwards.  There appear to be stray CRs (ASCII 015) in the
log files.  This is because I haven't as yet looked at the newline-
stripping section in the backquote substitution code (and maybe other
places as well).  Also, while typing `ls' at the shell will start up
`ls.ttp' just fine, typing `/bin/ls' will not work.  If I have another
day free, I will look into it.  It will be fixed RSN(tm). 

Another thing that won't work is the `#!' mechanism for executable
scripts of any kind.  In UNIX, when you attempt to execute a file, and
its first two characters are `#!', the kernel (!) looks in the first
line for an executable program and starts it with that file as an
argument.  For example, if the first line of your file reads

#! /bin/awk -f

then the file will be processed with awk.  Another common example is

#! /bin/sh

which will run your script with /bin/sh.  MiNT does not support this
feature, and although sh tries to emulate it, it will still not work,
because you cannot emulate executable permission on a plain text file
under MiNT (with the TOS filesystem at least).

The solution is of course to specify the interpreter for your scripts
explicitly.  For example, if you have a makefile that runs a shell
script just by mentioning its name, you should prefix the line with `sh'
and be done:

funcs.c funcs.h: mkfuncs
	mkfuncs		# This is a shell script

now becomes

funcs.c funcs.h: mkfuncs
	sh mkfuncs	# This is (still) a shell script



2.  Bells and Whistles
----------------------

Nearly every shell script that I have seen uses the program /bin/test. 
In fact, this is a trap that every UNIX programmer has fallen into at
least once; when one creates test.c, and compiles it into the executable
file test, and tries to run `test', one will mostly get /bin/test.  I
have written my own version of /bin/test (took me half a day or so), and
include it here in test.c.  The docs are in test.1  If you want to run
nonportable shell scripts on your machine, I suggest making a copy of
test.ttp under the name /bin/[.ttp (that's right, a square bracket). 
That is because shell scripts often say something like

  if [ -d /tmp ] ; then
    ...
  fi

What happens is that most UNIX systems have a link from /bin/[ to
/bin/test (or maybe [ ] is built into some shells).  I have written the
program with the man page for SUNs beside me, so it should be pretty
standard.

Other things that you should consider are the programs /bin/true
(returns EXIT_SUCCESS always) and /bin/false (returns EXIT_FAILURE
always). You should copy /bin/true.ttp to /bin/:.ttp.  (The programs are
so trivial that I haven't included them.  I considered posting source
code for /bin/true as an offense against the programming community :-)

If you make copies of the program(s) to give to your friends, please
read the copyright notice in the file COPYING (for test), and the
copyright notice in the file COPYING.sh (for sh).

Share and enjoy.

--
Stephan
