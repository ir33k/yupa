#define NAME "yupa"
#define VERSION "v4.0"

#include <assert.h>
#include <ctype.h>
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
#include "main.h"

#define TMP_RES     "/tmp/yupa.res"
#define TMP_OUT     "/tmp/yupa.out"

static char *envpager = "less -XI";
unsigned envmargin = 4;
unsigned envwidth = 76;

static void usage(char *argv0);
static char *loadpage(char *);
static void run(char *uri);

void
usage(char *argv0)
{
	printf("usage: %s [options] [URI]\n"
		"\n"
		"options:\n"
		"	-v	Print program version\n"
		"	-h	Print this help message\n"
		"\n"
		"URI:\n"
		"	Optional initial URI\n"
		"\n"
		"env:\n"
		"	YUPAPAGER	Pager program (%s)\n"
		"	YUPAMARGIN	Left margin (%d)\n"
		"	YUPAWIDTH	Max width (%d)\n",
	       argv0, envpager, envmargin, envwidth);
}

char *
loadpage(char *uri)
{
	char *why, *host, *path, buf[4096], *pt, *link;
	int protocol, port, ssl=0;
	FILE *fp;
	unsigned i;

	if (!uri)
		return "No URI";

	protocol = uri_protocol(uri);
	port = uri_port(uri);
	host = uri_host(uri);
	path = uri_path(uri);

	if (!port)
		port = protocol;

	switch (protocol) {
	case GOPHER:
		/* First part of the path holds resource type */
		if (path && strlen(path) > 2)
			path += 2;

		snprintf(buf, sizeof buf, "%s", path ? path : "");
		break;
	case GEMINI:
		snprintf(buf, sizeof buf, "gemini://%s%s",
			 host, path ? path : "/");
		ssl = 1;
		break;
	case HTTP:
		snprintf(buf, sizeof buf, "GET http://%s%s HTTP/1.0",
			 host, path ? path : "/");
		break;
	case HTTPS:
		snprintf(buf, sizeof buf, "GET %s HTTP/1.0\nHost: %s",
			 path ? path : "/", host);
		ssl = 1;
		break;
	default:
		return "Unknown protocol";
	}

	fprintf(stderr, "protocol: %d\n", protocol);
	fprintf(stderr, "port: %d\n", port);
	fprintf(stderr, "host: %s\n", host);
	fprintf(stderr, "path: %s\n", path);
	fprintf(stderr, "msg: %s\n", buf);

	fp = fopen(TMP_RES, "w+");
	if (!fp)
		err(1, "fopen(%s)", TMP_RES);

	why = fetch(host, port, ssl, buf, fp);

	if (why)
		return why;

	pt = fmalloc(fp);

	if (fclose(fp))
		err(1, "flose(%s)", TMP_RES);

	link_clear();
	link_store(uri);

	fp = fopen(TMP_OUT, "w");
	if (!fp)
		err(1, "fopen(%s)", TMP_OUT);

	switch (protocol) {
	case GOPHER: gph_print(pt, fp); break;
	case GEMINI: gmi_print(pt, fp); break;
	case HTTP:
	case HTTPS: html_print(pt, fp); break;
	}

	free(pt);

	fprintf(fp, "\n");
	fprintf(fp, "Links:\n");
	for (i=0; (link = link_get(i)); i++)
		fprintf(fp, "%u\t%s\n", i, link);

	if (fclose(fp))
		err(1, "flose(%s)", TMP_OUT);

	snprintf(buf, sizeof buf, "%s %s", envpager, TMP_OUT);
	system(buf);

	return 0;
}

void
run(char *uri)
{
	char buf[4096], *why=0, *link;

	if (uri)
		why = loadpage(uri);

	while (1) {
		if (why)
			printf(NAME": %s\n", why);

		printf(NAME"> ");
		fgets(buf, sizeof buf, stdin);
		trimr(buf);

		if (isdigit(buf[0])) {
			link = link_get(atoi(buf));
		} else {
			switch (buf[0]) {
			case 'q': case 'Q':
				exit(0);
			}
			link = buf;
		}
		uri = uri_normalize(link, link_get(0));
		why = loadpage(uri);
	}
}

int
main(int argc, char **argv)
{
	int opt, n;
	char *uri=0, *env;

	env = getenv("YUPAPAGER");
	if (env)
		envpager = env;

	env = getenv("YUPAMARGIN");
	n = env ? atoi(env) : 0;
	if (n > 0)
		envmargin = (unsigned)n;

	env = getenv("YUPAWIDTH");
	n = env ? atoi(env) : 0;
	if (n > (int)envmargin+4)
		envwidth = (unsigned)n;

	while ((opt = getopt(argc, argv, "vh")) != -1)
		switch (opt) {
		case 'v':
			puts(VERSION);
			return 0;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 1;
		}

	if (argc - optind > 0)
		uri = argv[argc-optind];

	run(uri);

	return 0;
}
