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

#define TMP_RES     "/tmp/yupa.res"
#define TMP_OUT     "/tmp/yupa.out"

static const char *help = "usage: %s URI";

static void run(char *uri);

void
run(char *uri)
{
	char *why, *host, *path, msg[4096], *buf, *link;
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

	res = fopen(TMP_RES, "w+");
	if (!res)
		errx(1, "fopen(%s)", TMP_RES);

	why = fetch(host, port, ssl, msg, res);

	if (why)
		err(1, "Error: %s", why);

	buf = fmalloc(res);

	if (fclose(res))
		err(1, "flose(%s)", TMP_RES);

	link_clear();
	link_store(uri);

	switch (protocol) {
	case GOPHER: gph_print(buf, stdout); break;
	case GEMINI: gmi_print(buf, stdout); break;
	case HTTP:
	case HTTPS: html_print(buf, stdout); break;
	}

	free(buf);

	fprintf(stdout, "\n");
	fprintf(stdout, "index\tlink\n");
	for (i=0; (link = link_get(i)); i++)
		fprintf(stdout, "%u\t%s\n", i, link);
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
