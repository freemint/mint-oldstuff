
#include "syslog.h"


main()
{
	int i;


	if( i = (openlog("syslog_test", LOG_PID | LOG_NDELAY, LOG_DAEMON)) < 0)
		printf("Openlog failed: %d\n", i);
	syslog(0, "log level 0");
	sleep(5);
	syslog(1, "log level 1");
	sleep(5);
	syslog(2, "log level 2");
	sleep(5);
	syslog(3, "log level 3");
	sleep(5);
	syslog(4, "log level 4");
	sleep(5);
	syslog(5, "log level 5");
	sleep(5);
	syslog(6, "log level 6");
	sleep(5);
	syslog(7, "log level 7");
	closelog();
}
