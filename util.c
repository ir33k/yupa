#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "util.h"

char *
tellmy(char *fmt, ...)
{
	static char buf[2][4096];
	static int i=0;
	va_list ap;

	/* NOTE(irek): Thanks to double buffering it's safe to chain
	 * this function without overwriting previous BUF.  With that
	 * it's possible to create errors stack.
	 *
	 *	why = tellmy("First error code %d", 42);
	 *	why = tellmy("Second error\n%s", why);
	 *	why = tellmy("Thirs error\n%s", why);
	 */
	i = !i;
	va_start(ap, fmt);
	vsnprintf(buf[i], sizeof buf[0], fmt, ap);
	va_end(ap);

	return buf[i];
}

char *
fmalloc(FILE *fp)
{
	char *pt;
	long n, m;

	if (fseek(fp, 0, SEEK_END))
		err(1, "fmalloc fseek");

	n = ftell(fp);
	pt = malloc(n +1);
	if (!pt)
		err(1, "fmalloc malloc(%ld)", n);

	rewind(fp);

	m = fread(pt, 1, n, fp);
	pt[m] = 0;

	if (m != n)
		errx(1, "fmalloc failed to read entire file");

	return pt;
}

char *
eachline(char **str)
{
	char *line;

	if (!**str)
		return 0;

	line = *str;

	while (**str && **str != '\n' && **str != '\r') (*str)++;
	if (**str) {
		if (**str == '\r') {
			**str = 0;
			(*str)++;
		}
		**str = 0;
		(*str)++;
	}

	return line;
}

char *
eachword(char **str)
{
	char *word;

	if (!**str)
		return 0;

	word = *str;

	while (**str && **str != ' ' && **str != '\t') (*str)++;
	if (**str) {
		**str = 0;
		(*str)++;
	}

	return word;
}

char *
triml(char *str)
{
	while (*str && *str <= ' ') str++;
	return str;
}

void
trimr(char *str)
{
	unsigned u;
	u = strlen(str);
	while (u && str[--u] <= ' ') str[u] = 0;
}
