#define __LIBRARY__
#include <unistd.h>

#define __NR_sys_syslog __NR_syslog

_syscall3(int,sys_syslog,int, type, char *, but, int, len);


