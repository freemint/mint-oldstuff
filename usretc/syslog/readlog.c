/*
 * Read log messages from the kernel - send them to syslogd
 *
 * Steve Lord (lord@cray.com) 7th Nov 92
 */


#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/syslog.h>

void	clean_up(int sig)
{
        /* remove the page in the kernel used for logging */
	sys_syslog(0, 0, 0);
	exit(0);
}

int	main()
{
	char	buffer[256], line[256], *cr, *ptr;
	int	len, index = 0;

	signal(SIGINT, clean_up);

	openlog("kernel", 0, LOG_KERN);

        /* start kernel logging */
	sys_syslog(1, NULL, 0);
	while (1) {
	        /* read from kernel log */
		len = sys_syslog(2, buffer, sizeof(buffer));
		ptr = buffer;
		while (len) {
			cr = strchr(ptr, '\n');
			if (cr) {
				len -= cr - ptr + 1;
				strncpy(line + index, ptr, cr - ptr);
				line[index + cr - ptr + 1] = '\0';
				syslog(LOG_ERR, line);
				ptr = cr + 1;
				index = 0;
			} else {
				strncat(line + index, ptr, len);
				index += len;
				len = 0;
			}
		}
	}
}

#define __LIBRARY__
#include <unistd.h>

#define __NR_sys_syslog __NR_syslog

_syscall3(int,sys_syslog,int, type, char *, but, int, len);

