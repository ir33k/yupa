#define NAME	"yupa"
#define VERSION	"v4.0"
#define AUTHOR	"irek@gabr.pl"

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mime.h"
#include "main.h"
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

#define SESSIONMAX 16	/* Arbitrary limit of sessions to avoid insanity */

char *  envhome    = "~/.yupa";
char *  envsession = "~/.yupa/0";
char *  envpager   = "less -XI +R";
char *  envimage   = "xdg-open";
char *  envvideo   = "xdg-open";
char *  envaudio   = "xdg-open";
char *  envpdf     = "xdg-open";
int     envmargin  = 4;
int     envwidth   = 76;

static char *pathlock;
static char *pathuri;
static char *pathres;
static char *pathbody;
static char *pathcache;
static char *pathcmd;
static char *pathinfo;

static void usage(char *argv0);
static void envstr(char *name, char **env);
static void envint(char *name, int *env);
static why_t loadpage(char *uri);
static void onprompt(char *);
static char *oncmd(char *);
static void run();
static void end() __attribute__((noreturn));
static void onsignal(int);
static char *startsession();

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
	       "	Optional initial input prompt value.\n"
	       "	Use URI/URL to load a page.\n"
	       "	Use \"help\" to learn about prompt commands.\n"
	       "\n"
	       "env:\n"
	       "	YUPAHOME     Absolute path to user data (%s).\n"
	       "	YUPASESSION  Runtime path to session dir (%s).\n"
	       "	YUPAPAGER    Overwrites $PAGER value (%s).\n"
	       "	YUPAIMAGE    Comand to display images (%s).\n"
	       "	YUPAVIDEO    Comand to play videos (%s).\n"
	       "	YUPAAUDIO    Comand to play audio (%s).\n"
	       "	YUPAPDF      Comand to open PDFs (%s).\n"
	       "	YUPAMARGIN   Left margin (%d).\n"
	       "	YUPAWIDTH    Max width (%d).\n",
	       argv0, envhome, envsession, envpager,
	       envimage, envvideo, envaudio, envpdf,
	       envmargin, envwidth);
}

void
envstr(char *name, char **env)
{
	char *str;

	if ((str = getenv(name)))
		*env = str;

	if (setenv(name, *env, 1))
		err(1, "setenv(%s)", name);
}

void
envint(char *name, int *env)
{
	char *str, buf[64];

	if ((str = getenv(name)))
		*env = atoi(str);

	snprintf(buf, sizeof buf, "%d", *env);
	if (setenv(name, buf, 1))
		err(1, "setenv(%s)", name);
}

why_t
loadpage(char *uri)
{
	char *host, *path, buf[4096], *cache, *pt;
	int protocol, port, ssl=0, mime;
	FILE *fp;

	if (!uri)
		return "No URI";

	protocol = uri_protocol(uri);
	port = uri_port(uri);
	host = uri_host(uri);
	path = uri_path(uri);

	if (!port)
		port = protocol;

	if ((cache = cache_get(uri))) {
		if (cp(cache, pathres))
			return tellme("Failed to load %s", uri);
	} else {
		switch (protocol) {
		case LOCAL:
			if (cp(path, pathres))
				return tellme("Failed to load local file %s", path);
			break;
		case GOPHER:
			pt = path;
			if (!pt) pt = "";
			if (strlen(pt) > 2) pt += 2;	/* Skip item type */
			snprintf(buf, sizeof buf, "%s", pt);
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
			return tellme("Unknown protocol %s", uri);
		}

		if (protocol != LOCAL) {
			if (fetch(host, port, ssl, buf, pathres))
				return tellme("Failed to load %s", uri);

			if (cache_add(uri, pathres))
				return tellme("Failed to load %s", uri);
		}
	}

	mime = mime_path(path);
	switch (protocol) {
	case HTTP:
	case HTTPS:
		break;
	case GEMINI:
		/* mime = gph_mime(pt); */
		break;
	case GOPHER:
		mime = gph_mime(path);
		break;
	}

	link_clear();
	link_store(uri);
	undo_add(uri);

	if (!(fp = fopen(pathuri, "w")))
		err(1, "fopen(%s)", pathuri);

	fprintf(fp, "%s", uri);

	if (fclose(fp))
		err(1, "flose(%s)", pathuri);

	switch (mime) {
	default:
	case BINARY:
		return tellme("Unsopported mime file type %s", uri);
	case TEXT:
		if (cp(pathres, pathbody))
			return tellme("Failed to load %s", uri);

		snprintf(buf, sizeof buf, "%s %s", envpager, pathbody);
		break;
	case GPH:
	case GMI:
	case HTML:
		if (!(fp = fopen(pathbody, "w")))
			err(1, "fopen(%s)", pathbody);

		pt = fmalloc(pathres);
		switch (mime) {
		case GPH: gph_print(pt, fp); break;
		case GMI: gmi_print(pt, fp); break;
		case HTML: html_print(pt, fp); break;
		}
		fprintf(fp, "\n");
		free(pt);

		if (fclose(fp))
			err(1, "flose(%s)", pathbody);

		snprintf(buf, sizeof buf, "%s %s", envpager, pathbody);
		break;
	case IMAGE:
		snprintf(buf, sizeof buf, "%s %s", envimage, pathbody);
		break;
	case VIDEO:
		snprintf(buf, sizeof buf, "%s %s", envvideo, pathbody);
		break;
	case AUDIO:
		snprintf(buf, sizeof buf, "%s %s", envaudio, pathbody);
		break;
	case PDF:
		snprintf(buf, sizeof buf, "%s %s", envpdf, pathbody);
		break;
	}

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
		link = oncmd(str);

	if (!link)
		return;

	uri = uri_normalize(link, link_get(0));
	why = loadpage(uri);

	if (why) {
		printf(NAME": %s\n", why);
		tellme(0);
	}
}

char *
oncmd(char *cmd)
{
	char buf[4096], *arg, *str=0;
	int i;
	FILE *fp;

	if (!cmd[0])
		return 0;

	arg = trim(cmd+1);

	if (cmd[0] >= 'A' && cmd[0] <= 'Z') {
		if (arg[0])
			bind_set(cmd[0], arg);
		else if ((str = bind_get(cmd[0])))
			onprompt(str);

		return 0;
	}
	
	switch (cmd[0]) {
	case 'q':
		end();
	case 'b':
		i = atoi(arg);
		return undo_go(i ? -i : -1);
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

		fp = fopen(pathinfo, "w");
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

		snprintf(buf, sizeof buf, "%s %s", envpager, pathinfo);
		system(buf);
		break;
	case '$':
		system(arg);
		break;
	case '!':
		snprintf(buf, sizeof buf, "%s %s", arg, pathbody);
		system(buf);
		break;
	case '|':
		snprintf(buf, sizeof buf, "<%s %s", pathbody, arg);
		system(buf);
		break;
	case '%':
		snprintf(buf, sizeof buf, "%s >%s", arg, pathcmd);
		system(buf);

		fp = fopen(pathcmd, "r");
		if (!fp)
			err(1, "fopen(/out)");

		fgets(buf, sizeof buf, fp);

		if (fclose(fp))
			err(1, "flose(/out)");

		onprompt(trim(buf));
		break;
	default:
		return cmd;	/* CMD is probably a relative URI */
	}

	return 0;
}

void
run()
{
	char buf[4096];

	while (1) {
		printf(NAME"> ");
		if (!fgets(buf, sizeof buf, stdin))
			break;

		onprompt(trim(buf));
	}
}

void
end()
{
	cache_cleanup();
	unlink(pathlock);
	exit(0);
}

void
onsignal(int sig)
{
	switch (sig) {
	case SIGTERM:
	case SIGINT:
		end();
	}
}

char *
startsession()
{
	char buf[4096];
	int i, fd;

	/* NOTE(irek): The idea is that we want to create new session
	 * directory only when necessary.  Session directory being the
	 * envsession (YUPASESSION) path to dir with browser session files.
	 *
	 * If there is previous session directory that is no longer
	 * used (is not locked) then we will locked it and use for
	 * this session.  Create new if no session is free.
	 *
	 *	1. Session is just an integer
	 *	2. Used for dir name in $YUPAHOME
	 *	3. Locked session have .lock file
	 *
	 *	$YUPAHOME/0/.lock
	 *	$YUPAHOME/1/.lock
	 *	$YUPAHOME/2/
	 *	$YUPAHOME/3/.lock
	 *	$YUPAHOME/4/
	 */

	for (i=0; i<SESSIONMAX; i++) {
		snprintf(buf, sizeof buf, "%s/%d", envhome, i);

		if (access(buf, F_OK)) {
			if (mkdir(buf, 0755))
				err(1, "startsession mkdir(%s)", buf);

			break;	/* New session */
		}

		if (access(join(buf, "/.lock"), F_OK))
			break;	/* Free session */
	}

	if (i == SESSIONMAX)
		errx(1, "Exceeded maximum number of sessions %d", SESSIONMAX);

	fd = creat(join(buf, "/.lock"), 0644);
	if (fd == -1)
		err(1, "startsession create(%s)", buf);

	close(fd);

	return strdup(buf);
}

int
main(int argc, char **argv)
{
	int opt;
	char *uri=0;
	struct sigaction sa;

	sa.sa_handler = onsignal;
	sigaction(SIGTERM, &sa, 0);
	sigaction(SIGINT, &sa, 0);

	envstr("YUPAHOME",   &envhome);
	envstr("PAGER",      &envpager);
	envstr("YUPAPAGER",  &envpager);
	envstr("YUPAIMAGE",  &envimage);
	envstr("YUPAVIDEO",  &envvideo);
	envstr("YUPAAUDIO",  &envaudio);
	envstr("YUPAPDF",    &envpdf);
	envint("YUPAMARGIN", &envmargin);
	envint("YUPAWIDTH",  &envwidth);

	envhome = strdup(resolvepath(envhome));
	mkdir(envhome, 0755);

	envsession = startsession();
	if (setenv("YUPASESSION", envsession, 1))
		err(1, "setenv(YUPASESSION)");

	if (envmargin < 0)
		errx(0, "YUPAMARGIN has to be >= 0");

	if (envwidth < envmargin)
		errx(0, "YUPAMARGIN has to be > YUPAMARGIN");

	while ((opt = getopt(argc, argv, "vh")) != -1)
		switch (opt) {
		case 'v':
			puts(NAME" "VERSION" by <"AUTHOR">");
			return 0;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 1;
		}

	pathlock  = strdup(join(envsession, "/.lock"));
	pathres   = strdup(join(envsession, "/res"));
	pathbody  = strdup(join(envsession, "/body"));
	pathcache = strdup(join(envsession, "/cache"));
	pathcmd   = strdup(join(envsession, "/cmd"));
	pathinfo  = strdup(join(envsession, "/info"));
	pathuri   = strdup(join(envsession, "/uri"));

	mkdir(pathcache, 0755);

	bind_init();

	if (argc - optind > 0)
		uri = argv[argc-optind];

	if (uri)
		onprompt(uri);

	run();
	end();
	return 0;
}
