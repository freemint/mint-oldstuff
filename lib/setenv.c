int
setenv (str)
	char *str;
{
	extern int putenv (char *);
	return putenv (str);
}
