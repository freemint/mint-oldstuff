/*
 * BSD initgroups() emulation. Written by Kay Roemer.
 */

#include <sys/types.h>
#include <grp.h>

int
initgroups (user, group)
	char *user;
	gid_t group;
{
/* Under MiNT, a process cannot have additional group id's. */
	return setgid (group);
}
