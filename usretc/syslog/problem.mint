There is a possible problem with MiNT or some versions of the Minix
filesystem device driver which may either cause MiNT to lock up or the
system log daemon to die with a "hardware" access violation when run on a
machine with memory protection.

If /var/adm/messages is read by a program whilst syslogd is running, the
next time syslogd tries to write to the file it will crash. If
/var/adm/messages is read by a program more than twice, on the third time
MiNT will seize up completely.

This problem was found whilst using MiNT 1.07 and Minixfs version 6.0
patchevel 4.

Stephen Usher, 9th August, 1993.
