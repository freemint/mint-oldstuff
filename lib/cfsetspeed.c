#include <sys/types.h>
#include <termios.h>

int
cfsetspeed (tt, baud)
	struct termios *tt;
	speed_t baud;
{
	cfsetispeed (tt, baud);
	cfsetospeed (tt, baud);
	return 0;
}
