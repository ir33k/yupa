#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fsp.h"
#include "gmi.h"
#include "gph.h"
#include "le.h"
#include "net.h"
#include "uri.h"
#include "util.h"

// Return protocol based on file NAME extension.
static enum uri
ext_protocol(char *name)
{
	char *ext;
	if (!(ext = strrchr(name, '.'))) {
		return URI_NUL;
	}
	if (!strcasecmp(ext, ".gph")) return URI_GOPHER;
	if (!strcasecmp(ext, ".gmi")) return URI_GEMINI;
	return URI_NUL;
}

enum net_res
fsp_req(FILE *raw, FILE *fmt, char *uri)
{
	enum net_res res;
	FILE *fp;
	char buf[4096], *path;
	path = uri + strlen("file://");
	if (!(fp = fopen(path, "r"))) {
		perror(0);
		return NET_ERR;
	}
	while (fgets(buf, sizeof(buf), fp)) {
		fputs(buf, raw);
	}
	if (fclose(fp)) {
		perror(0);
	}
	res = NET_FMT;
	switch (ext_protocol(uri)) {
	case URI_GOPHER: gph_fmt(raw, fmt); break;
	case URI_GEMINI: gmi_fmt(raw, fmt); break;
	case URI_FILE:
	case URI_FTP:
	case URI_SSH:
	case URI_FINGER:
	case URI_HTTP:
	case URI_HTTPS:
	case URI_NUL:
	default:
		res = NET_RAW;
	}
	return res;
}

char *
fsp_uri(char *uri, FILE *body, int index)
{
	switch (ext_protocol(uri)) {
	case URI_GOPHER: return gph_uri(body, index);
	case URI_GEMINI: return gmi_uri(body, index);
	case URI_FILE:
	case URI_FTP:
	case URI_SSH:
	case URI_FINGER:
	case URI_HTTP:
	case URI_HTTPS:
	case URI_NUL:
		break;
	}
	return 0;
}
