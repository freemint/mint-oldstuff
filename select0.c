/* select0 -- just a select() for stdin...

   run as fullscreen .TOS on an idle desktop to stop old GEMs sucking away
   cycles from processes on other terminals.
   returns at next keypress, the key is not eaten so it can already be
   the next desktop command for TOS >= 2.x.  (and you can put this on
   fkeys on TOS >= 2.x too.)  for a desktop no-op try undo or space...
*/

#include <mintbind.h>
#include <minimal.h>
#include <fcntl.h>

int main()
{
	long rfd = 1;

	/* put tty back in `pseudo-RAW' mode, to pass ^c etc. */
	(void) Fputchar (0, 0L, 0);
	return Fselect (0, &rfd, 0L, 0L) == 1;
}
