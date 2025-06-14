#include <assert.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include "uri.h"

static int str_starts_with(char *str, char *prefix);

int
str_starts_with(char *str, char *prefix)
{
	unsigned sn, pn;

	sn = str ? strlen(str) : 0;
	pn = prefix ? strlen(prefix) : 0;

	if (prefix == 0 || sn < pn)
		return 0;

	return !strncasecmp(str, prefix, pn);
}

int
uri_protocol(char *uri)
{
	assert(uri);
	if (str_starts_with(uri, "http://")) return HTTP;
	if (str_starts_with(uri, "https://")) return HTTPS;
	if (str_starts_with(uri, "gemini://")) return GEMINI;
	if (str_starts_with(uri, "gopher://")) return GOPHER;
	return 0;
}

char *
uri_host(char *uri)
{
	static char buf[4096];
	char *beg, *end;
	size_t n;

	assert(uri);

	beg = (beg = strstr(uri, "://")) ? beg+3 : uri;	/* Skip protocol */
	n = strlen(beg);

	if (!n)
		return 0;

	if (!(end = strpbrk(beg, ":/")))
		end = beg + n;

	n = end - beg;

	if (!n)
		return 0;

	buf[n] = 0;

	return memcpy(buf, beg, n);
}

int
uri_port(char *uri)
{
	char *beg;
	assert(uri);
	beg = (beg = strstr(uri, "://")) ? beg+3 : uri;	/* Skip protocol */
	return (beg = strchr(beg, ':')) ? atoi(beg+1) : 0;
}

char *
uri_path(char *uri)
{
	char *beg;
	assert(uri);
	beg = (beg = strstr(uri, "://")) ? beg+3 : uri;	/* Skip protocol */
	return strchr(beg, '/');
}
