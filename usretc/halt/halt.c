#include <stdio.h>
#include <signal.h>
#include <sys/syslog.h>

#ifdef MINT
#include <ioctl.h>
#include <sys/dir.h>

typedef unsigned long	ulong;

typedef struct _context {
	long	regs[15];	/* registers d0-d7, a0-a6 */
	long	usp;		/* user stack pointer (a7) */
	short	sr;		/* status register */
	long	pc;		/* program counter */
	long	ssp;		/* supervisor stack pointer */
	long	term_vec;	/* GEMDOS terminate vector (0x102) */

/* these fields were added in MiNT 0.9 */
	char	fstate[216];	/* FPU internal state */
	char	fregs[12*8];	/* registers fp0-fp7 */
	long	fctrl[3];		/* FPCR/FPSR/FPIAR */
	short	sfmt;			/* stack frame format identifier */
	long	iar;			/* instruction address */
	short	internal[4];	/* four words of internal info */
} CONTEXT;

struct pinfo {
	long	magic;
	char	*base;
	short	pid, ppid, pgrp;
	short	ruid, rgid;
	short	euid, egid;
	short	memflags;
	short	pri;
	short	wait_q;
	long	wait_cond;
	/* (all times are in milliseconds) */
	ulong	systime;		/* time spent in kernel		*/
	ulong	usrtime;		/* time spent out of kernel	*/
	ulong	chldstime;		/* children's kernel time 	*/
	ulong	chldutime;		/* children's user time		*/

	ulong	maxmem;			/* max. amount of memory to use */
	ulong	maxdata;		/* max. data region for process */
	ulong	maxcore;		/* max. core memory for process */
	ulong	maxcpu;			/* max. cpu time to use 	*/

	short	domain;			/* process domain (TOS or MiNT)	*/
	short	curpri;			/* current priority (nice)	*/
};
#endif

main(argc, argv)
int argc;
char *argv[];
{
#ifdef MINT
	int pid, initpid;
	DIR *procdir;
	struct direct *entry;
	char *pointer;
#endif

	if (getuid() != 0)
	{
		printf("You need to be root to run halt.\n");
		exit();
	}

	if (!strcmp(argv[0], "fasthalt"))
		creat("/etc/fastboot", 0600);

	openlog("halt", 0, LOG_AUTH);
	syslog(LOG_NOTICE, "halted by root");
	closelog();

#ifdef MINT
	if ((procdir = opendir("u:/proc")) == NULL)
	{
		perror("opendir: u:/proc");
	}
	else
	{
		/* look for process called init with ppid 0
		*/
		while ((entry = readdir(procdir)) != NULL)
		{
			char statf[0x80] = "u:/proc/";
			int fd;

			entry->d_name[entry->d_namlen] = '\0';
			pointer = strchr(entry->d_name, '.');
			sscanf(++pointer, "%3d", &pid);
			strcpy (statf+sizeof "u:/proc", entry->d_name);
			if (!strncmp
			    (entry->d_name, "init.", pointer-1-entry->d_name) &&
			    (fd = open (statf, 0)) >=0) {
				long place;
				long ctxtsize;
				struct pinfo proc;

				ioctl(fd, PPROCADDR, &place);
				if (ioctl(fd, PCTXTSIZE, &ctxtsize) < 0) {
					lseek(fd, place+4+2*sizeof(CONTEXT), 0);
				} else
					lseek(fd, place, 0);
				read(fd, &proc, sizeof(proc));
				if (proc.pid == pid && proc.ppid <= 0) {
					initpid = pid;
					break;
				}
			}
		}
		closedir(procdir);
	}

	if (kill(initpid, SIGTERM) == -1)
		perror("kill");
#else
	if (kill(1, SIGTERM) == -1)
		perror("kill");
#endif

	sleep(1);
}
