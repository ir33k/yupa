#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "util.h"
#include "uri.h"

static void skip(char *str, unsigned n);
static char *protocol_str(int);

void
skip(char *str, unsigned n)
{
	unsigned i;

	for (i=0; str[n]; i++, n++)
		str[i] = str[n];

	str[i] = 0;
}

char *
protocol_str(int protocol)
{
	switch (protocol) {
	case LOCAL: return "file";
	case HTTP: return "http";
	case HTTPS: return "https";
	case GEMINI: return "gemini";
	case GOPHER: return "gopher";
	}
	return 0;
}

int
uri_protocol(char *uri)
{
	assert(uri);
	if (startswith(uri, "file://")) return LOCAL;
	if (startswith(uri, "http://")) return HTTP;
	if (startswith(uri, "https://")) return HTTPS;
	if (startswith(uri, "gemini://")) return GEMINI;
	if (startswith(uri, "gopher://")) return GOPHER;
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
	static char buf[4096];
	char *beg;
	assert(uri);
	beg = (beg = strstr(uri, "://")) ? beg+3 : uri;	/* Skip protocol */
	beg = strchr(beg, '/');
	return beg ? strcpy(buf, beg) : 0;
}

char *
uri_normalize(char *link, char *base)
{
	static char buf[4096];
	int protocol, port;
	char *host, *path, *prefix, *pt, *tmp;

	if (!base)
		base = "";

	if (link[0] == '/') {	/* Link is an absolute path */
		protocol = uri_protocol(base);
		port = uri_port(base);
		host = uri_host(base);
		path = link;
	} else if (strstr(link, "://")) {	/* Link is a full URI */
		protocol = uri_protocol(link);
		port = uri_port(link);
		host = uri_host(link);
		path = uri_path(link);
	} else {	/* Link is a relative path */
		protocol = uri_protocol(base);
		port = uri_port(base);
		host = uri_host(base);

		if ((pt = strrchr(base, '/')))
			*(pt+1) = 0;

		snprintf(buf, sizeof buf, "%s%s", base, link);
		path = uri_path(buf);
	}

	if (!host)
		host = "";

	if (!protocol)
		protocol = port;

	prefix = protocol_str(protocol);

	/* NOTE(irek): Protocol, and by extension the prefix, should
	 * always be defined.  Without it it's not possible for the
	 * link to be normalized.  Usually protocol is present thanks
	 * to base URI but it's still possible that first link typed
	 * by user will not have it and there will be no port to guess
	 * the protocol. */
	if (!prefix)
		return 0;

	if (!port)
		port = protocol;

	if (!path)
		path = "";

	/* Resolve "//" "./" and "../" */
	if (path[0]) {
		while ((pt = strstr(path, "//")))
			path = pt + 1;

		while ((pt = strstr(path, "../"))) {
			if (pt == path)
				break;

			tmp = pt-1;
			if (*tmp == '/')
				tmp--;

			while (tmp > path && *tmp != '/')
				tmp--;

			skip(tmp, (pt-tmp) + 2);
		}

		pt = path;
		while ((pt = strstr(pt, "./")))
			skip(pt, 2);
	}

	snprintf(buf, sizeof buf, "%s://%s:%d%s", prefix, host, port, path);
	return buf;
}
