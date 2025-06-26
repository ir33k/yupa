#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "util.h"

char *
tellme(char *why, char *fmt, ...)
{
	static char buf[2][4096];
	static int i=0;
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(buf[i], sizeof buf[0], fmt, ap);
	va_end(ap);

	if (why && (unsigned)n < sizeof buf[0])
		snprintf(buf[i]+n, (sizeof buf[0])-n, "\n%s", why);

	return buf[i];
}

char *
join(char *a, char *b)
{
	static char buf[4096];
	snprintf(buf, sizeof buf, "%s%s", a, b);
	return buf;
}

char *
fmalloc(char *path)
{
	FILE *fp;
	char *pt;
	long n, m;

	if (!(fp = fopen(path, "r")))
		err(1, "fmalloc fopen");

	if (fseek(fp, 0, SEEK_END))
		err(1, "fmalloc fseek");

	n = ftell(fp);

	if (!(pt = malloc(n +1)))
		err(1, "fmalloc malloc(%ld)", n);

	rewind(fp);

	m = fread(pt, 1, n, fp);
	pt[m] = 0;

	if (m != n)
		errx(1, "fmalloc failed to read entire file");

	if (fclose(fp))
		err(1, "fmalloc fclose %s", path);

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

char *
trim(char *str)
{
	trimr(str);
	return triml(str);
}

char *
cp(char *from, char *to)
{
	char *why=0, buf[BUFSIZ];
	FILE *fp0, *fp1;
	size_t n;

	if (!(fp0 = fopen(from, "r")))
		return tellme(0, "Failed to open %s", from);

	if (!(fp1 = fopen(to, "w"))) {
		why = tellme(0, "Failed to open %s", to);
		goto fail0;
	}

	while ((n = fread(buf, 1, sizeof buf, fp0)))
		if (fwrite(buf, 1, n, fp1) != n) {
			why = tellme(0, "Failed to write %lu bytes"
				     "from %s to %s",
				     n, from, to);
			goto fail1;
		}

fail1:	if (fclose(fp1))
		why = tellme(why, "Failed to close %s", to);

fail0:	if (fclose(fp0))
		why = tellme(why, "Failed to close %s", from);

	return why;
}
