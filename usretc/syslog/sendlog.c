/*
 * SENDLOG
 *
 * Used in shell scripts to send messages to the system log -- ASP
 */

#include "syslog.h"

main(argc, argv)
	int	argc;
	char	**argv;
{
	int i;
	char buf[200];
	buf[0] = '\0';
	for (i = 1; i < argc; i++)
		strcat(buf, argv[i]);
	openlog("sendlog", 0, LOG_USER);
	syslog(LOG_INFO, buf);
}
