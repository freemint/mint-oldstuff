#include <unistd.h>

int
chroot (path)
	char *path;
{
	return chdir (path);
}
