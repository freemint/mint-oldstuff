/*
 * lastlog.h 1.0 for MiNT, taken from MiNTOS V1.3
 */

#ifndef _LASTLOG_H_
#define _LASTLOG_H_

struct lastlog {
	time_t	ll_time;
	char	ll_line[8];
	char	ll_host[16];
};

#endif /* _LASTLOG_H_ */
