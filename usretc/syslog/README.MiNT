
                           Syslogd and Syslog.o

These are the syslogd and syslog ported from 4.3BSD (that's the new one with
the *very* flexible config file).  

Syslogd is essentially unchanged from the 4.3BSD. You can configure it (via
/etc/syslog.conf) to log messages in different logfiles (depending upon the
sender's facility code and the priority), log to users' terminals (same
dependancies), and if things get real bad, it can do a wall (write-all; same
dependancies).

Syslog is really only modified in that it uses a MiNT fifo instead of
sockets for inter-process communications.  See syslog.h for facility codes
and priorities that can be used.


BUGS:
Messages from facilities LOG_KERN, LOG_USER, and LOG_PRT never can be
wall-ed, no matter how high the priority.  I'm still trying to decide if
this is a bug or a feature. :-)


ALSO INCLUDED:

Syslog_test, sendlog (to use from shell scripts, untested) and logger (for
use in shell script also, not able to be compiled until strcasecmp() written).

NEEDED:

GCC 2.x where x < 4. GCC 2.4.x is currently terminally buggy for the m68k
	family of processors, use GCC 2.3.3 instead.

Mintlibs version 30 or later.

-----------------------------------------------------------------------

To the MiNT library co-ordinators:-

It would be a good idea to include syslog.c and syslog.h in the library and
include distributions, as they are in the BSD library.
