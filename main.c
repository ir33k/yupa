#include <assert.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "util.h"
#include "uri.h"
#include "fetch.h"
#include "link.h"
#include "gmi.h"
#include "gph.h"
#include "html.h"

#define CRLF            "\r\n"
#define TMP_RES_RAW     "/tmp/yupa.res.raw"
#define TMP_RES_TXT     "/tmp/yupa.res.txt"

static const char *help = "usage: %s URI";

static char *fmalloc(FILE *);
static char *get_body(int protocol, char *response);
static void run(char *uri);

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
get_body(int protocol, char *res)
{
	char *body;

	body = res;

	/* Skip response headers if present depending of protocol to find body beginning */
	switch (protocol) {
	case HTTP:
	case HTTPS:
		body = strstr(res, CRLF CRLF);
		if (body)
			body += strlen(CRLF)*2;
		break;
	case GEMINI:
		body = strstr(res, CRLF);
		if (body)
			body += 2;
		break;
	}

	if (!body)
		body = res;

	return body;
}

void
run(char *uri)
{
	char *why, *host, *path, msg[4096], *str, *body, *link;
	int protocol, port, ssl=0;
	FILE *res;
	unsigned i;

	protocol = uri_protocol(uri);
	port = uri_port(uri);
	host = uri_host(uri);
	path = uri_path(uri);

	if (!protocol)
		protocol = port;

	if (!port)
		port = protocol;

	switch (protocol) {
	case GOPHER:
		/* First part of the path holds resource type */
		if (path && strlen(path) > 2)
			path += 2;

		snprintf(msg, sizeof msg, "%s", path ? path : "");
		break;
	case GEMINI:
		snprintf(msg, sizeof msg, "gemini://%s%s",
			 host, path ? path : "/");
		ssl = 1;
		break;
	case HTTP:
		snprintf(msg, sizeof msg, "GET http://%s%s HTTP/1.0",
			 host, path ? path : "/");
		break;
	case HTTPS:
		snprintf(msg, sizeof msg, "GET %s HTTP/1.0\nHost: %s",
			 path ? path : "/", host);
		ssl = 1;
		break;
	default:
		errx(1, "Unknown protocol");
		break;
	}

	fprintf(stderr, "protoc	%d\n", protocol);
	fprintf(stderr, "host	%s\n", host);
	fprintf(stderr, "port	%d\n", port);
	fprintf(stderr, "ssl	%d\n", ssl);
	fprintf(stderr, "msg	%s\n", msg);

	res = fopen(TMP_RES_RAW, "w+");
	if (!res)
		errx(1, "fopen(%s)", TMP_RES_RAW);

	why = fetch(host, port, ssl, msg, res);

	if (why)
		err(1, "Error: %s", why);

	/* TODO(irek): At this point I should parse response header,
	 * response code, or response metadata depending on protocol.
	 * It's important because depending on response or in case of
	 * Gopher depending on request parsing is optional. */

	str = fmalloc(res);

	if (fclose(res))
		err(1, "flose(%s)", TMP_RES_RAW);

	body = get_body(protocol, str);

	link_clear();
	link_store(uri);

	switch (protocol) {
	case GOPHER: gph_print(body, stdout); break;
	case GEMINI: gmi_print(body, stdout); break;
	case HTTP:
	case HTTPS: html_print(body, stdout); break;
	}

	fprintf(stdout, "\n");
	for (i=0; (link = link_get(i)); i++)
		fprintf(stdout, "[%d] %s\n", i, link);

	free(str);
}

int
main(int argc, char **argv)
{
	char *uri;

	if (argc < 2)
		errx(0, help, argv[0]);

	uri = argv[1];

	run(uri);

	return 0;
}
