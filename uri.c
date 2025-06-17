#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "uri.h"

static int starts_with(char *str, char *prefix);
static char *protocol_str(int);

int
starts_with(char *str, char *prefix)
{
	unsigned n = strlen(prefix);
	return n > strlen(str) ? 0 : !strncasecmp(str, prefix, n);
}

char *
protocol_str(int protocol)
{
	switch (protocol) {
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
	if (starts_with(uri, "http://")) return HTTP;
	if (starts_with(uri, "https://")) return HTTPS;
	if (starts_with(uri, "gemini://")) return GEMINI;
	if (starts_with(uri, "gopher://")) return GOPHER;
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
	char *host, *path, *prefix;

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
		snprintf(buf, sizeof buf, "%s%s", base, link);
		protocol = uri_protocol(buf);
		port = uri_port(buf);
		host = uri_host(buf);
		path = uri_path(buf);
	}

	if (!protocol)
		protocol = port;

	prefix = protocol_str(protocol);

	/* NOTE(irek): Protocol, and by extension the prefix, should
	 * always be defined.  Without it it's not possible for the
	 * link to be normalized.  Usually protocol is always present
	 * thanks to base URI but it's still possible that first link
	 * typed by user will not have it and there will be no port
	 * either to guess the protocol. */
	if (!prefix)
		return 0;

	if (!port)
		port = protocol;

	if (!path)
		path = "";

	snprintf(buf, sizeof buf, "%s://%s:%d%s", prefix, host, port, path);
	return buf;
}
