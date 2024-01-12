#include <stdio.h>
#include "uri.h"

char *
uri_protocol_str(enum uri protocol)
{
	switch (protocol) {
	case URI_NUL:    return "<NULL>";
	case URI_GOPHER: return "gopher";
	case URI_FINGER: return "finger";
	case URI_GEMINI: return "gemini";
	case URI_HTTPS:  return "https";
	case URI_HTTP:   return "http";
	case URI_FILE:   return "file";
	case URI_ABOUT:  return "about";
	case URI_SSH:    return "ssh";
	case URI_FTP:    return "ftp";
	}
	return "";
}

int
uri_protocol(char *uri)
{
	char *beg;
	int sz;
	if (!uri) {
		return URI_NUL;
	}
	if (!(beg = strstr(uri, "://"))) {
		return URI_NUL;
	}
	if ((sz = beg - uri) <= 0) {
		return URI_NUL;
	}
#define _URI_IS(x) sz == strlen(x) && !strncasecmp(uri, x, sz)
	if (_URI_IS("gopher")) return URI_GOPHER;
	if (_URI_IS("finger")) return URI_FINGER;
	if (_URI_IS("gemini")) return URI_GEMINI;
	if (_URI_IS("https"))  return URI_HTTPS;
	if (_URI_IS("http"))   return URI_HTTP;
	if (_URI_IS("file"))   return URI_FILE;
	if (_URI_IS("about"))  return URI_ABOUT;
	if (_URI_IS("ssh"))    return URI_SSH;
	if (_URI_IS("ftp"))    return URI_FTP;
	return URI_NUL;
}

char *
uri_host(char *uri)
{
	static char buf[URI_SZ];
	char *beg, *end;
	size_t len;
	assert(uri);
	assert(strlen(uri) < URI_SZ);
	beg = (beg = strstr(uri, "://")) ? beg + 3 : uri; // Skip protocol
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

int
uri_port(char *uri)
{
	char *beg;
	assert(uri);
	beg = (beg = strstr(uri, "://")) ? beg + 3 : uri; // Skip protocol
	return (beg = strchr(beg, ':')) ? atoi(beg + 1) : 0;
}

char *
uri_path(char *uri)
{
	char *beg;
	assert(uri);
	beg = (beg = strstr(uri, "://")) ? beg + 3 : uri; // Skip protocol
	return strchr(beg, '/');
}

char *
uri_norm(int protocol, char *host, int port, char *path)
{
	static char uri[URI_SZ];
	assert(host);
	if (!path) {
		path = "";
	}
	if (*path == '/') {
		path++;
	}
	if (!port) {
		port = protocol;
	}
	snprintf(uri, sizeof(uri), "%s://%s:%d/%s",
		uri_protocol_str(protocol), host, port, path);
	return uri;
}

int
uri_abs(char *uri)
{
	return uri && strstr(uri, "://");
}
