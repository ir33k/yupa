#define NAME "yupa"
#define VERSION "v4.0"

#define _POSIX_C_SOURCE 200809L	/* For mkdtemp */

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "util.h"
#include "uri.h"
#include "fetch.h"
#include "link.h"
#include "undo.h"
#include "gmi.h"
#include "gph.h"
#include "html.h"
#include "main.h"

char envtmp[] = "/tmp/"NAME"XXXXXX";
char *envhome = "~/."NAME;
char *envpager = "less -XI";
unsigned envmargin = 4;
unsigned envwidth = 76;

static void usage(char *argv0);
static char *join(char *, char *);
static char *loadpage(char *);
static char *oncmd(char *);
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
		"	YUPATMP         Runtime tmp dir with session files (%s)\n"
		"	YUPAHOME        Dir with persistent user data (%s)\n"
		"	YUPAPAGER       Pager program (%s)\n"
		"	YUPAMARGIN      Left margin (%d)\n"
		"	YUPAWIDTH       Max width (%d)\n",
	       argv0, envtmp, envhome, envpager, envmargin, envwidth);
}

char *
join(char *a, char *b)
{
	static char buf[4096];
	snprintf(buf, sizeof buf, "%s%s", a, b);
	return buf;
}

char *
loadpage(char *uri)
{
	char *why, *host, *path, buf[4096], *pt;
	int protocol, port, ssl=0;
	FILE *fp;

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

	fp = fopen(join(envtmp, "/res"), "w+");
	if (!fp)
		err(1, "fopen(res)");

	why = fetch(host, port, ssl, buf, fp);

	if (why)
		return why;

	pt = fmalloc(fp);

	if (fclose(fp))
		err(1, "flose(res)");

	link_clear();
	link_store(uri);
	undo_add(uri);

	fp = fopen(join(envtmp, "/uri"), "w");
	if (!fp)
		err(1, "fopen(uri)");

	fprintf(fp, "%s", uri);

	if (fclose(fp))
		err(1, "flose(out)");

	fp = fopen(join(envtmp, "/body"), "w");
	if (!fp)
		err(1, "fopen(body)");

	switch (protocol) {
	case GOPHER: gph_print(pt, fp); break;
	case GEMINI: gmi_print(pt, fp); break;
	case HTTP:
	case HTTPS: html_print(pt, fp); break;
	}
	fprintf(fp, "\n");

	free(pt);

	if (fclose(fp))
		err(1, "flose(out)");

	snprintf(buf, sizeof buf, "%s %s", envpager, join(envtmp, "/body"));
	system(buf);

	return 0;
}

char *
oncmd(char *cmd)
{
	char *arg, *link;
	int i;

	if (!cmd[0])
		return 0;

	arg = triml(cmd+1);
	
	switch (cmd[0]) {
	case 'q':
		exit(0);
	case 'b':
		i = atoi(arg);
		return undo_go(i ? i : -1);
	case 'f':
		i = atoi(arg);
		return undo_go(i ? i : 1);
	case 'i':
		if (arg[0]) {
			if (arg[0] >= 'A' && arg[0] <= 'Z') {
				printf("bind %c\n", arg[0]);
			} else {
				i = atoi(arg);
				printf("%s\n", link_get(i));
			}
		} else {
			// TODO(irek): Use pager
			printf("Links:\n");
			for (i=0; (link = link_get(i)); i++)
				printf("%u\t%s\n", i, link);

			printf("Binds:\n");
		}
		break;
	case '$':
		printf("$ cmd\n");
		break;
	case '!':
		printf("! cmd\n");
		break;
	case '|':
		printf("| cmd\n");
		break;
	case '%':
		printf("%% cmd\n");
		break;
	default:
		return cmd;	/* CMD is probably a relative URI */
	}

	return 0;
}

void
run(char *uri)
{
	char buf[2][4096]={0}, *why=0, *link;
	int i=0, last=0;

	if (uri)
		uri = uri_normalize(uri, 0);

	if (uri)
		why = loadpage(uri);

	while (1) {
		if (why)
			printf(NAME": %s\n", why);

		why = 0;
		printf(NAME"> ");
		fgets(buf[i], sizeof buf[0], stdin);
		trimr(buf[i]);

		if (!buf[i][0])
			i = last;

		if (isdigit(buf[i][0]))
			link = link_get(atoi(buf[i]));
		else
			link = oncmd(triml(buf[i]));

		last = i;
		i = !i;

		if (!link)
			continue;

		uri = uri_normalize(link, link_get(0));
		why = loadpage(uri);
	}
}

int
main(int argc, char **argv)
{
	int opt, n;
	char *uri=0, *env;

	if (!mkdtemp(envtmp))
		err(1, "mkdtemp");

	if (setenv("YUPATMP", envtmp, 1))
		err(1, "setenv(YUPATMP)");

	env = getenv("YUPAHOME");
	if (env)
		envhome = env;

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
