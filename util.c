#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include "util.h"

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
	pt[m+1] = 0;

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
triml(char *str)
{
	while (*str && *str <= ' ') str++;
	return str;
}
