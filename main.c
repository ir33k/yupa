#define NAME "yupa"
#define VERSION "v4.0"
#define AUTHOR "irek@gabr.pl"

#define _POSIX_C_SOURCE	200112L	/* For setenv */

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
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

#define SESSIONMAX 16	/* Arbitrary limit of sessions to avoid insanity */

enum mime { UNKNOWN=0, TXT, GPH, GMI, HTML };

char *envhome;
char *envsession;
char *envpager = "less -XI +R";
unsigned envmargin = 4;
unsigned envwidth = 76;

static char *path_lock;
static char *path_uri;
static char *path_res;
static char *path_body;
static char *path_cache;
static char *path_cmd;
static char *path_info;

static void usage(char *argv0);
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
	       "	Optional initial input prompt value like URI.\n"
	       "\n"
	       "env:\n"
	       "	YUPAHOME	Absolute path to user data (%s).\n"
	       "	YUPASESSION	Runtime path to session dir (%s).\n"
	       "	YUPAPAGER	Overwrites $PAGER value (%s).\n"
	       "	YUPAMARGIN	Left margin (%d).\n"
	       "	YUPAWIDTH	Max width (%d).\n",
	       argv0, envhome, envsession, envpager, envmargin, envwidth);
}

why_t
loadpage(char *uri)
{
	why_t why;
	char *host, *path, buf[4096], *cache, *pt;
	int protocol, port, ssl=0, mime=0;
	FILE *fp;

	if (!uri)
		return "No URI";

	protocol = uri_protocol(uri);
	port = uri_port(uri);
	host = uri_host(uri);
	path = uri_path(uri);

	if (!port)
		port = protocol;

	if (protocol == LOCAL) {
		if ((why = cp(path, path_res)))
			return tellme(why, "Failed to load local file %s", path);

		pt = strrchr(path, '.');
		if (!pt) mime = 0;
		else if (strcasecmp(pt, ".txt"))  mime = TXT;
		else if (strcasecmp(pt, ".html")) mime = HTML;
		else if (strcasecmp(pt, ".gmi"))  mime = GMI;
		else if (strcasecmp(pt, ".gph"))  mime = GPH;
	} else {
		cache = cache_get(uri);

		if (cache) {
			if ((why = cp(cache, path_res)))
				return tellme(why, "Failed to load %s", uri);
		} else {
			switch (protocol) {
			case GOPHER:
				mime = GPH;
				/* First part of the path holds resource type */
				if (path && strlen(path) > 2)
					path += 2;

				snprintf(buf, sizeof buf, "%s", path ? path : "");
				break;
			case GEMINI:
				mime = GMI;
				snprintf(buf, sizeof buf, "gemini://%s%s",
					 host, path ? path : "/");
				ssl = 1;
				break;
			case HTTP:
				mime = HTML;
				snprintf(buf, sizeof buf, "GET http://%s%s HTTP/1.0",
					 host, path ? path : "/");
				break;
			case HTTPS:
				mime = HTML;
				snprintf(buf, sizeof buf, "GET %s HTTP/1.0\nHost: %s",
					 path ? path : "/", host);
				ssl = 1;
				break;
			default:
				return tellme(0, "Unknown protocol %s", uri);
			}

			if (!(fp = fopen(path_res, "w+")))
				err(1, "fopen(%s)", path_res);

			if ((why = fetch(host, port, ssl, buf, fp)))
				return tellme(why, "Failed to load %s", uri);

			// TODO(irek): Early return skips this fclose()
			if (fclose(fp))
				err(1, "flose(%s)", path_res);
		}
	}

	link_clear();
	link_store(uri);
	undo_add(uri);

	if (!(fp = fopen(path_uri, "w")))
		err(1, "fopen(%s)", path_uri);

	fprintf(fp, "%s", uri);

	if (fclose(fp))
		err(1, "flose(%s)", path_uri);

	if ((why = cache_add(uri, path_res)))
		return tellme(why, "Failed to load %s", uri);

	if (!mime)
		return tellme(0, "Unsopported mime file type %s", uri);

	if (mime == TXT) {
		if ((why = cp(path_res, path_body)))
			return tellme(why, "Failed to load %s", uri);
	} else {
		if (!(fp = fopen(path_body, "w")))
			err(1, "fopen(%s)", path_body);

		pt = fmalloc(path_res);
		switch (mime) {
		case GPH: gph_print(pt, fp); break;
		case GMI: gmi_print(pt, fp); break;
		case HTML: html_print(pt, fp); break;
		}
		fprintf(fp, "\n");
		free(pt);

		if (fclose(fp))
			err(1, "flose(%s)", path_body);
	}

	snprintf(buf, sizeof buf, "%s %s", envpager, path_body);
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

	arg = trim(cmd+1);

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

		fp = fopen(path_info, "w");
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

		snprintf(buf, sizeof buf, "%s %s", envpager, path_info);
		system(buf);
		break;
	case '$':
		system(arg);
		break;
	case '!':
		snprintf(buf, sizeof buf, "%s %s", arg, path_body);
		system(buf);
		break;
	case '|':
		snprintf(buf, sizeof buf, "<%s %s", path_body, arg);
		system(buf);
		break;
	case '%':
		snprintf(buf, sizeof buf, "%s >%s", arg, path_cmd);
		system(buf);

		fp = fopen(path_cmd, "r");
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
	unlink(path_lock);
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
	int opt, n;
	char *uri=0, *env;
	struct passwd *pw;
	struct sigaction sa;

	sa.sa_handler = onsignal;
	sigaction(SIGTERM, &sa, 0);
	sigaction(SIGINT, &sa, 0);

	envhome = getenv("YUPAHOME");
	if (!envhome) {
		/* Default to ~/.yupa (but use absolute path) */
		pw = getpwuid(getuid());
		envhome = strdup(join(pw ? pw->pw_dir : "", "/."NAME));
	}
	mkdir(envhome, 0755);

	if (setenv("YUPAHOME", envhome, 1))
		err(1, "setenv(YUPAHOME)");

	envsession = startsession();

	if (setenv("YUPASESSION", envsession, 1))
		err(1, "setenv(YUPASESSION)");

	path_lock  = strdup(join(envsession, "/.lock"));
	path_res   = strdup(join(envsession, "/res"));
	path_body  = strdup(join(envsession, "/body"));
	path_cache = strdup(join(envsession, "/cache"));
	path_cmd   = strdup(join(envsession, "/cmd"));
	path_info  = strdup(join(envsession, "/info"));
	path_uri   = strdup(join(envsession, "/uri"));

	mkdir(path_cache, 0755);

	if ((env = getenv("PAGER")))
		envpager = env;

	if ((env = getenv("YUPAPAGER")))
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
			puts(NAME" "VERSION" by <"AUTHOR">");
			return 0;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 1;
		}

	bind_init();

	if (argc - optind > 0)
		uri = argv[argc-optind];

	if (uri)
		onprompt(uri);

	run();
	end();
	return 0;
}
