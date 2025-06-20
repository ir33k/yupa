#define NAME "yupa"
#define VERSION "v4.0"

#define _POSIX_C_SOURCE	200809L	/* For mkdtemp */

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <getopt.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"
#include "uri.h"
#include "fetch.h"
#include "link.h"
#include "undo.h"
#include "bind.h"
#include "gmi.h"
#include "gph.h"
#include "html.h"
#include "cache.h"
#include "main.h"

char envtmp[] = "/tmp/"NAME"XXXXXX";
char *envhome = 0;
char *envpager = "less -XI +R";
unsigned envmargin = 4;
unsigned envwidth = 76;

static void usage(char *argv0);
static char *join(char *, char *);
static void end() __attribute__((noreturn));
static char *loadpage(char *);
static void onprompt(char *);
static char *oncmd(char *);
static void run(char *);
static void sig_cb(int);

void
usage(char *argv0)
{
	printf("usage: %s [options] [prompt]\n"
		"\n"
		"options:\n"
		"	-v	Print program version.\n"
		"	-h	Print this help message.\n"
		"\n"
		"prompt:\n"
		"	Optional initial input prompt value, URI for example.\n"
		"\n"
		"env:\n"
		"	YUPATMP         Runtime tmp dir with session files (%s).\n"
		"	YUPAHOME        Dir with persistent user data (%s).\n"
		"	YUPAPAGER       Owerwrites $PAGER value (%s).\n"
		"	YUPAMARGIN      Left margin (%d).\n"
		"	YUPAWIDTH       Max width (%d).\n",
	       argv0, envtmp, envhome, envpager, envmargin, envwidth);
}

char *
join(char *a, char *b)
{
	static char buf[4096];
	snprintf(buf, sizeof buf, "%s%s", a, b);
	return buf;
}

void
end()
{
	exit(0);
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

	/* pt = cache_get(uri); */
	/* if (pt) { */
	/* } */

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

	if (protocol == LOCAL) {
		snprintf(buf, sizeof buf, "cp %s %s", path, join(envtmp, "/res"));
		system(buf);
	} else {
		fp = fopen(join(envtmp, "/res"), "w+");
		if (!fp)
			err(1, "fopen(/res)");

		why = fetch(host, port, ssl, buf, fp);

		if (why)
			return why;

		pt = fmalloc(fp);

		if (fclose(fp))
			err(1, "flose(/res)");
	}

	link_clear();
	link_store(uri);
	undo_add(uri);
	cache_add(uri, join(envtmp, "/res"));

	fp = fopen(join(envtmp, "/uri"), "w");
	if (!fp)
		err(1, "fopen(/uri)");

	fprintf(fp, "%s", uri);

	if (fclose(fp))
		err(1, "flose(/uri)");

	fp = fopen(join(envtmp, "/body"), "w");
	if (!fp)
		err(1, "fopen(/body)");

	switch (protocol) {
	case GOPHER: gph_print(pt, fp); break;
	case GEMINI: gmi_print(pt, fp); break;
	case HTTP:
	case HTTPS: html_print(pt, fp); break;
	}
	fprintf(fp, "\n");

	free(pt);

	if (fclose(fp))
		err(1, "flose(/body)");

	snprintf(buf, sizeof buf, "%s %s", envpager, join(envtmp, "/body"));
	system(buf);

	return 0;
}

void
onprompt(char *str)
{
	char *why=0, *link, *uri;

	if (!str || !str[0])
		return;

	if (isdigit(str[0]))
		link = link_get(atoi(str));
	else
		link = oncmd(triml(str));

	if (!link)
		return;

	uri = uri_normalize(link, link_get(0));
	why = loadpage(uri);

	if (why)
		printf(NAME": %s\n", why);
}

char *
oncmd(char *cmd)
{
	char buf[4096], *arg, *str;
	int i;
	FILE *fp;

	if (!cmd[0])
		return 0;

	arg = triml(cmd+1);

	if (cmd[0] >= 'A' && cmd[0] <= 'Z') {
		if (arg[0]) {
			bind_set(cmd[0], arg);
		} else {
			str = bind_get(cmd[0]);
			if (!str)
				return 0;
			onprompt(str);
		}
		return 0;
	}
	
	switch (cmd[0]) {
	case 'q':
		end();
	case 'b':
		i = atoi(arg);
		return undo_go(i ? i : -1);
	case 'f':
		i = atoi(arg);
		return undo_go(i ? i : 1);
	case 'i':
		if (arg[0]) {
			if (arg[0] >= 'A' && arg[0] <= 'Z')
				str = bind_get(arg[0]);
			else
				str = link_get(atoi(arg));

			if (str)
				printf("%s\n", str);

			break;
		}

		fp = fopen(join(envtmp, "/info"), "w");
		if (!fp)
			err(1, "fopen(/info)");

		for (i=0; (str = link_get(i)); i++)
			fprintf(fp, "%u\t%s\n", i, str);

		for (i='A'; i<='Z'; i++) {
			str = bind_get(i);
			if (str)
				fprintf(fp, "%c\t%s\n", i, str);
		}

		if (fclose(fp))
			err(1, "flose(/info)");

		snprintf(buf, sizeof buf, "%s %s", envpager, join(envtmp, "/info"));
		system(buf);
		break;
	case '$':
		system(arg);
		break;
	case '!':
		snprintf(buf, sizeof buf, "%s %s", arg, join(envtmp, "/body"));
		system(buf);
		break;
	case '|':
		snprintf(buf, sizeof buf, "<%s %s", join(envtmp, "/body"), arg);
		system(buf);
		break;
	case '%':
		snprintf(buf, sizeof buf, "%s >%s", arg, join(envtmp, "/out"));
		system(buf);

		fp = fopen(join(envtmp, "/out"), "r");
		if (!fp)
			err(1, "fopen(/out)");

		fgets(buf, sizeof buf, fp);

		if (fclose(fp))
			err(1, "flose(/out)");

		trimr(buf);
		onprompt(triml(buf));
		break;
	default:
		return cmd;	/* CMD is probably a relative URI */
	}

	return 0;
}

void
run(char *uri)
{
	char buf[4096];

	if (uri)
		onprompt(uri);

	while (1) {
		printf(NAME"> ");
		if (!fgets(buf, sizeof buf, stdin))
			break;

		trimr(buf);
		onprompt(buf);
	}
	printf("\n");
	end();
}

void
sig_cb(int sig)
{
	if (sig == SIGTERM || sig == SIGINT) {
		printf("\n");
		end();
	}
}

int
main(int argc, char **argv)
{
	int opt, n;
	char *uri=0, *env;
	struct passwd *pw;
	struct sigaction sa;

	sa.sa_handler = sig_cb;
	sigaction(SIGTERM, &sa, 0);
	sigaction(SIGINT, &sa, 0);

	if (!mkdtemp(envtmp))
		err(1, "mkdtemp");

	if (setenv("YUPATMP", envtmp, 1))
		err(1, "setenv(YUPATMP)");

	mkdir(join(envtmp, "/cache"), 0755);

	env = getenv("YUPAHOME");
	if (env)
		envhome = env;

	if (!envhome) {
		pw = getpwuid(getuid());
		envhome = strdup(join(pw ? pw->pw_dir : "", "/."NAME));
	}
	mkdir(envhome, 0755);

	if (setenv("YUPAHOME", envhome, 1))
		err(1, "setenv(YUPAHOME)");

	env = getenv("PAGER");
	if (env)
		envpager = env;

	env = getenv("YUPAPAGER");
	if (env)
		envpager = env;

	if (setenv("YUPAPAGER", envpager, 1))
		err(1, "setenv(YUPAPAGER)");

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
			puts(NAME" "VERSION);
			return 0;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 1;
		}

	bind_load(join(envhome, "/binds"));

	if (argc - optind > 0)
		uri = argv[argc-optind];

	run(uri);
	return 0;
}
