/*
 * ac - login accounting
 * (C) 1992 D P Gymer
 * $Log: ac.c,v $
 * Revision 0.2  1992/01/28  18:34:32  dpg
 * Altered printout format to match the SunOS version.
 *
 * Revision 0.1  1992/01/25  19:09:16  dpg
 * Initial revision.
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>

#define WTMP	"/var/adm/wtmp"

char rcsid[] = "$Id: ac.c,v 0.2 1992/01/28 18:34:32 dpg Exp $";

#define NSIZE	8
#define NCPY(n1, n2)	(strncpy(n1, n2, NSIZE))
#define NCMP(n1, n2)	(strncmp(n1, n2, NSIZE))
#define BLINE(rec)	(!strncmp("~", rec->ut_line, 8))

/* ASSUMPTION - every day has the same number of seconds. */
#define SECSPERDAY	(24L * 60L * 60L)
#define SECSPERHOUR	(60L * 60L)

#define USER	0x0001
#define DAILY	0x0002
int flags = 0;

struct time {
	time_t total;
	time_t last;
	char name[NSIZE];	/* Magic number alert! */
	struct time *next;
};

static void
outtime(date, name, time)
	char *date;
	char *name;
	time_t time;
{
	if (!date)
		date = "";
	if (!name)
		name = "total";
	printf("%-8.8s %-8.8s %4ld.%02ld\n", date, name, time / SECSPERHOUR,
		((time % SECSPERHOUR) * 100L) / SECSPERHOUR);
}

static void
check_boot(list, rec)
	struct time *list;
	struct utmp *rec;
{
	if (!BLINE(rec))
		return;
	while (list) {
		if (list->last
#ifdef SHUTDOWN_LOGIN
			&& (NCMP(list->name, "shutdown") ||
			!NCMP(rec->ut_name, "reboot"))
#endif
			)
		{
			list->total += rec->ut_time - list->last;
			list->last = 0;
		}
		list = list->next;
	}
}

static struct time *
update_time(rec, name, list)
	struct utmp *rec;
	char *name;
	struct time *list;
{
	struct time **timep = &list;

	while (*timep && NCMP((*timep)->name, name))
		timep = &(*timep)->next;
	if (!*timep)
		if (!(*timep = (struct time *) malloc(sizeof (struct time)))) {
			perror("ac");
			exit(1);
		} else {
			NCPY((*timep)->name, name);
			(*timep)->total = (*timep)->last = 0;
			(*timep)->next = 0;
		}
	if (*rec->ut_name)
		(*timep)->last = rec->ut_time;
	else if ((*timep)->last) {
		(*timep)->total += rec->ut_time - (*timep)->last;
		(*timep)->last = 0;
	}
	return list;
}

static void
print_time(list, current, date)
	struct time *list;
	time_t current;
	char *date;
{
	time_t total = 0;

	while (list) {
		if (list->last) {	/* Still logged on. */
			list->total += current - list->last;
			list->last = current;
		}
		if (list->total && flags & USER)
			outtime((char *) 0, list->name, list->total);
		total += list->total;
		list->total = 0;
		list = list->next;
	}
	outtime(date, (char *) 0, total);
}

static int
same_day(t1, t2)
	time_t t1;
	time_t t2;
{
	return t1 / SECSPERDAY != t2 / SECSPERDAY;
}

static void
new_day(list, newtime)
	struct time *list;
	time_t newtime;
{
	char date[16];
	time_t ltime;

	if (newtime) {
		newtime -= newtime % SECSPERDAY;
		ltime = newtime - 1;
	} else
		ltime = time(&newtime);
	strftime(date, 16, "%b %d", localtime(&ltime));
	print_time(list, newtime, date);
}

static int
ignore_user(users, name, rec)
	char **users;
	char *name;
	struct utmp *rec;
{
	if (BLINE(rec))
		return 1;
	if (!*users)
#ifdef SHUTDOWN_LOGIN
		return !NCMP(name, "shutdown");
#else
		return 0;
#endif
	while (*users)
		if (!NCMP(*users++, name))
			return 0;
	return 1;
}

static int
perform_ac(users, file)
	char **users;
	char *file;
{
	int fd;
	struct utmp rec;
	char who[NSIZE];
	struct time *list = 0;
	time_t ltime = 0;

	if ((fd = open(file, O_RDONLY)) < 0) {
		perror(file);
		return 1;
	}
	while (read(fd, &rec, sizeof(struct utmp)) == sizeof(struct utmp)) {
		if (*rec.ut_name)
			NCPY(who, rec.ut_name);
		if (ltime && flags & DAILY && same_day(ltime, rec.ut_time))
			new_day(list, rec.ut_time);
		check_boot(list, &rec);
		ltime = rec.ut_time;
		if (!ignore_user(users, who, &rec))
			list = update_time(&rec, who, list);
	}
	close(fd);
	if (flags & DAILY)
		new_day(list, (time_t) 0);
	else
		print_time(list, time((time_t *) 0), (char *) 0);
	return 0;
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	int c;
	char *wtmp = WTMP;
	extern int optind;
	extern char *optarg;

	while ((c = getopt(argc, argv, "dpw:")) != EOF)
		switch (c) {
		case 'd':
			flags |= DAILY;
			break;
		case 'p':
			flags |= USER;
			break;
		case 'w':
			wtmp = optarg;
			break;
		default:
			return 1;
		}

	return perform_ac(argv + optind, wtmp);
}
