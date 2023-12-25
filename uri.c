#include <stdio.h>
#include "uri.h"

char *
uri_protocol_str(enum uri_protocol protocol)
{
	switch (protocol) {
	case URI_PROTOCOL_NONE:	  return "NONE";
	case URI_PROTOCOL_NUL:	  return "";
	case URI_PROTOCOL_GOPHER: return "gopher";
	case URI_PROTOCOL_GEMINI: return "gemini";
	case URI_PROTOCOL_HTTPS:  return "https";
	case URI_PROTOCOL_HTTP:	  return "http";
	case URI_PROTOCOL_FILE:	  return "file";
	case URI_PROTOCOL_SSH:	  return "ssh";
	case URI_PROTOCOL_FTP:	  return "ftp";
	}
	assert(0 && "unreachable");
}

/* Get protocol from URI string. */
enum uri_protocol
uri_protocol(char *uri)
{
	char *beg;
	int siz;
	assert(uri);
	if (!(beg = strstr(uri, "://"))) {
		return URI_PROTOCOL_NUL;
	}
	if ((siz = beg - uri) <= 0) {
		return URI_PROTOCOL_NUL;
	}
#define _URI_PROTOCOL_IS(x) siz == strlen(x) && !strncasecmp(uri, x, siz)
	if (_URI_PROTOCOL_IS("gopher")) return URI_PROTOCOL_GOPHER;
	if (_URI_PROTOCOL_IS("gemini")) return URI_PROTOCOL_GEMINI;
	if (_URI_PROTOCOL_IS("https"))  return URI_PROTOCOL_HTTPS;
	if (_URI_PROTOCOL_IS("http"))   return URI_PROTOCOL_HTTP;
	if (_URI_PROTOCOL_IS("file"))   return URI_PROTOCOL_FILE;
	if (_URI_PROTOCOL_IS("ssh"))    return URI_PROTOCOL_SSH;
	if (_URI_PROTOCOL_IS("ftp"))    return URI_PROTOCOL_FTP;
	return URI_PROTOCOL_NONE;
}

/* Get host from URI string.  Return NULL if not found.  Return
 * pointer to static string on success. */
char *
uri_host(char *uri)
{
	static char buf[URI_SIZ];
	char *beg, *end;
	size_t len;
	assert(uri);
	assert(strlen(uri) < URI_SIZ);
	beg = (beg = strstr(uri, "://")) ? beg + 3 : uri; /* Skip protocol */
	len = strlen(beg);
	if (len == 0) {
		return 0;
	}
	if (!(end = strpbrk(beg, ":/"))) {
		end = beg + len;
	}
	len = end - beg;
	if (len == 0) {
		return 0;
	}
	buf[len] = 0;
	return memcpy(buf, beg, len);
}

/* Get port from URI string.  Return 0 by default. */
int
uri_port(char *uri)
{
	char *beg;
	assert(uri);
	beg = (beg = strstr(uri, "://")) ? beg + 3 : uri; /* Skip protocol */
	return (beg = strchr(beg, ':')) ? atoi(beg + 1) : 0;
}

/* Return pointer to first URI string path character.  Return NULL if
 * not found. */
char *
uri_path(char *uri)
{
	char *beg;
	assert(uri);
	beg = (beg = strstr(uri, "://")) ? beg + 3 : uri; /* Skip protocol */
	return strchr(beg, '/');
}

/* Construct normalized URI in DST buffer from given URI parts, where
 * only PATH can be NULL. */
void
uri_normalize(char dst[URI_SIZ], enum uri_protocol protocol, char *host,
	      int port, char *path)
{
	assert(dst);
	assert(protocol != URI_PROTOCOL_NONE);
	assert(host);
	assert(port > 0);
	sprintf(dst, "%s://%s:%d%s",
		uri_protocol_str(protocol),
		host, port, path ? path : "/");
}
