#include <assert.h>
#include <err.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "util.h"

char *
tellme(char *fmt, ...)
{
	static char buf[4096];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);

	return buf;
}

char *
join(char *a, char *b)
{
	static char buf[4096];
	snprintf(buf, sizeof buf, "%s%s", a, b);
	return buf;
}

char *
resolvepath(char *path)
{
	static char buf[4096];
	char *pt, *home="";
	struct passwd *pwd;

	path = trim(path);
	
	if (!path[0])
		return "/";

	while ((pt = strstr(path, "//")))
		path = pt +1;

	while ((pt = strstr(path +1, "~/")))
		path = pt;

	if (path[0] == '~' && (pwd = getpwuid(getuid()))) {
		home = pwd->pw_dir;
		path++;
	}

	snprintf(buf, sizeof buf, "%s%s", home, path);
	return buf;
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
trim(char *str)
{
	unsigned u;

	// Trim left
	while (*str && *str <= ' ') str++;

	// Trim right
	u = strlen(str);
	while (u && str[--u] <= ' ') str[u] = 0;

	return str;
}

why_t
fcp(FILE *from, FILE *to)
{
	char buf[BUFSIZ];
	size_t n;

	while ((n = fread(buf, 1, sizeof buf, from)))
		if (fwrite(buf, 1, n, to) != n)
			return "Failed to copy files";

	return 0;
}

why_t
cp(char *from, char *to)
{
	char *why=0;
	FILE *fp0, *fp1;

	if (!(fp0 = fopen(from, "r")))
		return tellme("Failed to open %s", from);

	if (!(fp1 = fopen(to, "w"))) {
		why = tellme("Failed to open %s", to);
		goto fail;
	}

	why = fcp(fp0, fp1);

	if (fclose(fp1))
		why = tellme("Failed to close %s", to);

fail:	if (fclose(fp0))
		why = tellme("Failed to close %s", from);

	return why;
}

int
startswith(char *str, char *prefix)
{
	if (!str) str = "";
	unsigned n = strlen(prefix);
	return n > strlen(str) ? 0 : !strncasecmp(str, prefix, n);
}
