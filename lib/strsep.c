/*
 *	strsep() - TeSche
 */

#include <stdio.h>
#include <string.h>

char *strsep(char **s, char *sep)
{
	register char	*ptr, *this = *s;

	if (*s) {
		if ((ptr = strpbrk(*s, sep))) {
			*ptr++ = 0;
			*s = ptr;
		} else {
			*s = NULL;
		}

		return this;
	}

	return NULL;
}
