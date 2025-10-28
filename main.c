#define NAME	"yupa"			/* Yet another village is dead */
#define VERSION	"v6.1"
#define AUTHOR	"irek@gabr.pl"

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "main.h"
#include "txt.h"
#include "gmi.h"
#include "gph.h"

#define CRLF		"\r\n"
#define SESSIONSN	16		/* Arbitrary limit to avoid insanity */
#define BINDSN		('Z'-'A'+1)	/* Inclusive range from A to Z */
#define UNDOSN		32

static char *help =
	"q/e	Quit / Exit\n"
	"g uri	Goto URI\n"
	"b [n]	Go back in browsing history by 1 or N\n"
	"f [n]	Go forward in browsing history by 1 or N\n"
	"l [x]	List page links or link X\n"
	"r	View raw response\n"
	"! cmd	Run CMD\n"
	"%% cmd	Use CMD stdout as input\n"
	"A-Z [x]	Binds, invoke or define with X\n";

enum { LOCAL=4, HTTP=80, HTTPS=443, GEMINI=1965, GOPHER=70 };

typedef char		Undo[4096];
typedef struct cache	Cache;

struct cache {
	char*	key;
	int	age;
};

static void	usage		(char *argv0);
static void	envstr		(char *name, char **env);
static void	envint		(char *name, int *env);
static Why	loadpage	(char *link);
static void	onprompt	(char*);
static void	end		(int) __attribute__((noreturn));
static void	onsignal	(int);
static char*	startsession	();
static char*	join		(char*, char*);
static char*	resolvepath	(char*);
static Why	fcp		(FILE *src, FILE *dst);
static Why	cp		(char *src, char *dst);
static void	skip		(char *str, unsigned n);

static Why	fetch_secure	(int sfd, char *host, char *msg, FILE *out);
static Why	fetch_plain	(int sfd, char *msg, FILE *out);
static Why	fetch		(char *host, int port, int ssl, char *msg, char *out);

static int	bind_init	();
static void	bind_set	(char bind, char *str);
static char*	bind_get	(char bind);

static char*	cache_path	(int index);
static Why	cache_add	(char *key, char *path);
static char*	cache_get	(char *key);

static void	link_clear	();
static char*	link_get	(int i);

static char*	uri_protocolstr	(int protocol);
static int	uri_protocol	(char *uri);
static char*	uri_host	(char *uri);
static int	uri_port	(char *uri);
static char*	uri_path	(char *uri);
static char*	uri_normalize	(char *link);

static void	undo_add	(char *uri);
static char*	undo_goto	(int offset);

static Mime	mime_path	(char *path);
static Mime	mime_header	(char *str);

static char*	binds[BINDSN]	= {0};
static Cache	caches[36]	= {0};
static char**	links		= 0;
static int	linksn		= 0;
static int	linkscap	= 0;
static Undo	undos[UNDOSN]	= {0};
static int	undo_last	= 0;

static char*	envhome		= "~/.yupa";
static char*	envsession	= "~/.yupa/0";
static char*	envpager	= "less -XI +R";
static char*	envhtml		= "xdg-open";

int	envmargin	= 4;
int	envwidth	= 76;

static char*	pathlock;
static char*	pathres;
static char*	pathout;
static char*	pathlinks;
static char*	pathcmd;
static char*	pathcache;
static char*	pathbinds;
static char*	pathhistory;
static char*	pathbook;

void
usage(char *argv0)
{
	printf("usage: %s [options] [URI/cmd/help]\n"
	       "\n",
	       argv0);

	printf("options:\n"
	       "	-v		Print program version\n"
	       "	-h		Print this help message\n"
	       "\n");

	printf("envs:\n"
	       "	YUPAHOME	Absolute path to user data (%s)\n"
	       "	YUPASESSION	Runtime path to session dir (%s)\n"
	       "	YUPAURI		Runtime URI of current page\n"
	       "	YUPAPAGER	Overwrites $PAGER value (%s)\n"
	       "	YUPAHTML	Command to open websites (%s)\n"
	       "	YUPAMARGIN	Left margin (%d)\n"
	       "	YUPAWIDTH	Max width (%d)\n",
	       envhome, envsession, envpager, envhtml, envmargin, envwidth);
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

Why
loadpage(char *link)
{
	Why why;
	char uri[4096], buf[4096];
	char *host, *path, *cache, *search, *pt, *header;
	int protocol, port, ssl;
	Mime mime;
	int n;
	FILE *res, *out;

	if (!link)
		return "No link";

	why = 0;
	pt = uri_normalize(link);
	snprintf(uri, sizeof uri, "%s", pt);

	protocol = uri_protocol(uri);
	port = uri_port(uri);
	host = uri_host(uri);
	path = uri_path(uri);

	if (!port) port = protocol;

	if ((cache = cache_get(uri))) {
		if ((why = cp(cache, pathres)))
			return why;
	} else {
		ssl = 0;
		buf[0] = 0;

		switch (protocol) {
		case LOCAL:
			if ((why = cp(path, pathres)))
				return why;
			break;
		case GOPHER:
			if ((search = gph_search(path))) {
				n = strlen(uri);
				snprintf(uri+n, (sizeof uri) -n, "%s", search);
				path = uri_path(uri);
			}

			snprintf(buf, sizeof buf, "%s",
				 path[1] ? path +2 : path);
			break;
		case GEMINI:
			snprintf(buf, sizeof buf, "gemini://%s%s",
				 host, path ? path : "/");
			ssl = 1;
			break;
		case HTTP:
		case HTTPS:
			snprintf(buf, sizeof buf, "%s %s", envhtml, uri);
			system(buf);
			return 0;
		default:
			return "Unknown protocol";
		}

		if (buf[0]) {
			if ((why = fetch(host, port, ssl, buf, pathres)))
				return why;

			if ((why = cache_add(uri, pathres)))
				return why;
		}
	}

	if (!(res = fopen(pathres, "r")))
		err(1, "fopen(%s)", pathres);

	mime = 0;
	switch (protocol) {
	case LOCAL:
		mime = mime_path(path);
		break;
	case GOPHER:
		mime = gph_mime(path);
		break;
	case GEMINI:
		if ((search = gmi_search(pt)))
			return loadpage(search);

		pt = 0;
		if ((why = gmi_onheader(res, &header, &pt)))
			return why;

		if (header)
			mime = mime_header(header);

		if (!pt)
			break;

		/* Redirect */

		if (fclose(res))
			err(1, "flose(%s)", pathres);

		link_clear();
		link_store(uri);

		return loadpage(pt);
	case HTTP:
	case HTTPS:
		assert(0 && "unreachable");
	}

	link_clear();
	link_store(uri);
	undo_add(uri);

	if (setenv("YUPAURI", uri, 1))
		err(1, "setenv(YUPAURI)");

	if (!(out = fopen(pathout, "w")))
		err(1, "fopen(%s)", pathout);

	switch (mime) {
	default: case 0:  why = "Unsupported mime file type"; break;
	case MIME_BINARY: why = "Binary mime file type is unsupported"; break;
	case MIME_GPH:    gph_print(res, out); break;
	case MIME_GMI:    gmi_print(res, out); break;
	case MIME_TEXT:   txt_print(res, out); break;
	}

	if (fclose(res))
		err(1, "flose(%s)", pathres);

	if (fclose(out))
		err(1, "flose(%s)", pathout);

	if (!why) {
		snprintf(buf, sizeof buf, "<%s %s", pathout, envpager);
		system(buf);
	}

	return why;
}

void
onprompt(char *input)
{
	Why why;
	char cmd, *arg, *tmp, buf[4096];
	int i, delta;
	FILE *fp;

	assert(input);
	input = trim(input);

	why = 0;
	cmd = 0;
	arg = 0;
	delta = 1;

	if (!input[0]) {
		return;
	} else if (isdigit(input[0])) {
		cmd = 'g';
		arg = link_get(atoi(input));
	} else if (uri_protocol(input)) {
		cmd = 'g';
		arg = input;
	} else {
		cmd = input[0];
		arg = triml(input+1);
	}

	if (arg && !arg[0])
		arg = 0;

	if (cmd >= 'A' && cmd <= 'Z') {		/* Binds */
		if (arg)
			bind_set(cmd, arg);
		else if ((arg = bind_get(cmd)))
			onprompt(arg);
		return;
	}

	switch (cmd) {
	case 'h':
		printf(help);

		for (i='A'; i<='Z'; i++)
			if ((tmp = bind_get(i)))
				printf("%c\t%s\n", i, tmp);

		break;
	case 'q':	/* quit */
	case 'e':	/* end */
		end(0);
	case 'g':	/* go */
		if (!arg)
			why = "Invalid link";
		else
			why = loadpage(arg);
		break;
	case 'b':	/* back */
		delta = -1;
		/* fallthrough */
	case 'f':	/* forward */
		arg = undo_goto((arg ? atoi(arg) : 1) * delta);
		why = loadpage(arg);
		break;
	case 'l':	/* links */
		if (arg) {
			if (isdigit(arg[0]))
				arg = link_get(atoi(arg));

			if (arg)
				printf("%s\n", arg);
			else
				why = "Invalid info argument";

			break;
		}

		fp = fopen(pathlinks, "w");
		if (!fp)
			err(1, "fopen(/info)");

		for (i=0; (tmp = link_get(i)); i++)
			fprintf(fp, "%u\t%s\n", i, tmp);

		if (fclose(fp))
			err(1, "flose(/info)");

		snprintf(buf, sizeof buf, "<%s %s", pathlinks, envpager);
		system(buf);
		break;
	case 'r':	/* raw */
		snprintf(buf, sizeof buf, "<%s %s", pathres, envpager);
		system(buf);
		break;
	case '!':	/* cmd */
		system(arg);
		break;
	case '%':	/* meta cmd */
		snprintf(buf, sizeof buf, "%s >%s", arg, pathcmd);
		system(buf);

		fp = fopen(pathcmd, "r");
		if (!fp)
			err(1, "fopen(/out)");

		fgets(buf, sizeof buf, fp);

		if (fclose(fp))
			err(1, "flose(/out)");

		onprompt(buf);
		break;
	default:
		why = "Unknown command";
	}

	if (why)
		printf("%s\n", why);
}

void
end(int code)
{
	int i;

	for (i=0; i<LENGTH(caches); i++)
		if (caches[i].key)
			unlink(cache_path(i));

	rmdir(pathcache);
	unlink(pathlock);
	unlink(pathlinks);
	unlink(pathcmd);
	exit(code);
}

void
onsignal(int sig)
{
	switch (sig) {
	case SIGTERM:
	case SIGINT:
		end(0);
	}
}

char *
startsession()
{
	char buf[4096];
	int i, fd;

	/* YUPASESSION is a path to directory inside YUPAHOME that
	 * is unique for running process.  Session is a number being
	 * directory name inside YUPAHOME.  Running / taken session
	 * directories are locked with .lock file.
	 *
	 *	$YUPAHOME/0/.lock	Currently used session
	 *	$YUPAHOME/1/.lock
	 *	$YUPAHOME/2/		Old session that can be reused
	 *	$YUPAHOME/3/.lock
	 *	$YUPAHOME/4/
	 *
	 * In this scenario returned value will be $YUPAHOME/2
	 */

	for (i=0; i<SESSIONSN; i++) {
		snprintf(buf, sizeof buf, "%s/%d", envhome, i);

		if (access(buf, F_OK)) {
			if (mkdir(buf, 0755))
				err(1, "startsession mkdir(%s)", buf);

			break;	/* Create new session */
		}

		if (access(join(buf, "/.lock"), F_OK))
			break;	/* Found free session */
	}

	if (i == SESSIONSN)
		errx(1, "Exceeded maximum number of sessions %d", SESSIONSN);

	fd = creat(join(buf, "/.lock"), 0644);
	if (fd == -1)
		err(1, "startsession create(%s)", buf);

	close(fd);

	return strdup(buf);
}

char *
join(char *a, char *b)
{
	static char buf[4096];
	snprintf(buf, sizeof buf, "%s%s", a, b);
	return buf;
}

char *
resolvepath(char *path)
{
	static char buf[4096];
	char *pt, *home="";
	struct passwd *pwd;

	path = trim(path);

	if (!path[0])
		return "/";

	while ((pt = strstr(path, "//")))
		path = pt +1;

	while ((pt = strstr(path +1, "~/")))
		path = pt;

	if (path[0] == '~' && (pwd = getpwuid(getuid()))) {
		home = pwd->pw_dir;
		path++;
	}

	snprintf(buf, sizeof buf, "%s%s", home, path);
	return buf;
}

char *
eachword(char **str)
{
	char *word;

	if (!**str)
		return 0;

	word = *str;

	while (**str && **str > ' ') (*str)++;

	if (**str) {
		**str = 0;
		(*str)++;
	}

	return word;
}

char *
triml(char *str)
{
	while (*str && (unsigned)*str <= ' ') str++;
	return str;
}

void
trimr(char *str)
{
	int u = strlen(str);
	while (u && (unsigned)str[--u] <= ' ') str[u] = 0;
}

char *
trim(char *str)
{
	str = triml(str);
	trimr(str);
	return str;
}

Why
fcp(FILE *src, FILE *dst)
{
	char buf[BUFSIZ];
	size_t n;

	while ((n = fread(buf, 1, sizeof buf, src)))
		if (fwrite(buf, 1, n, dst) != n)
			return "Failed to copy files";

	return 0;
}

Why
cp(char *src, char *dst)
{
	char *why=0;
	FILE *fp0, *fp1;

	if (!(fp0 = fopen(src, "r")))
		return "Failed to open src";

	if (!(fp1 = fopen(dst, "w"))) {
		why = "Failed to open dst";
		goto fail;
	}

	why = fcp(fp0, fp1);

	if (fclose(fp1))
		why = "Failed to close dst";

fail:	if (fclose(fp0))
		why = "Failed to close src";

	return why;
}

int
starts(char *str, char *with)
{
	int n = strlen(with);
	if (!str) str = "";
	return n > (int)strlen(str) ? 0 : !strncasecmp(str, with, n);
}

void
skip(char *str, unsigned n)
{
	unsigned i;

	for (i=0; str[n]; i++, n++)
		str[i] = str[n];

	str[i] = 0;
}

Why
fetch_secure(int sfd, char *host, char *msg, FILE *out)
{
	static SSL_CTX *ctx=0;
	static SSL *ssl=0;

	char buf[4096];
	size_t n;

	assert(sfd);
	assert(host);
	assert(msg);

	/*
	 * NOTE(irek): This is an easy way of avoiding memory leak.
	 * By having both variables as static I can check on every
	 * next function usage if there is something to free.  It's
	 * true that there will be an unused memory laying around
	 * after last function usage but this is not a big deal.  It's
	 * more important to avoid memory leak and this is a very
	 * simple and elegant way of doing that in C where normally it
	 * would be a mess with many return statements.  I use the
	 * same trick for sfd in fetch().
	 */
	SSL_CTX_free(ctx);
	ctx = 0;
	SSL_free(ssl);
	ssl=0;

	if (!(ctx = SSL_CTX_new(TLS_client_method())))
		return "Failed to create SSL context";

	if (!(ssl = SSL_new(ctx)))
		return "Failed to create SSL instance";

	if (!SSL_set_tlsext_host_name(ssl, host))
		return "Failed to TLS set hostname";

	if (!SSL_set_fd(ssl, sfd))
		return "Failed to set SSL sfd";

	if (SSL_connect(ssl) < 1)
		return "Failed to stablish SSL connection";

	if (SSL_write(ssl, msg, strlen(msg)) < 1)
		return "Failed to send secure message to server";

	if (SSL_write(ssl, CRLF, sizeof CRLF) < 1)
		return "Failed to send secure CRLF to server";

	while ((n = SSL_read(ssl, buf, sizeof buf)))
		if (fwrite(buf, 1, n, out) != (size_t)n)
			return "Failed to write entire response";

	return 0;
}

Why
fetch_plain(int sfd, char *msg, FILE *out)
{
	char buf[BUFSIZ];
	ssize_t n;

	if (send(sfd, msg, strlen(msg), 0) == -1)
		return "Failed to send request to server";

	if (send(sfd, CRLF, sizeof(CRLF), 0) == -1)
		return "Failed to send CRLF to server";

	while ((n = recv(sfd, buf, sizeof buf, 0)))
		if (fwrite(buf, 1, n, out) != (size_t)n)
			return "Failed to write entire response";

	return 0;
}

Why
fetch(char *host, int port, int ssl, char *msg, char *out)
{
	Why why;
	static int sfd=-1;
	int i;
	struct hostent *he;
	struct sockaddr_in addr;
	FILE *fp;

	assert(host);
	assert(port > 0);
	assert(msg);

	if (sfd >= 0 && close(sfd))
		return "Failed to close socket";

	if ((sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		return "Failed to open socket";

	if ((he = gethostbyname(host)) == 0)
		return "Failed to get hostname";

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	for (i=0; he->h_addr_list[i]; i++) {
		memcpy(&addr.sin_addr.s_addr, he->h_addr_list[i], sizeof(in_addr_t));
		if (!connect(sfd, (struct sockaddr*)&addr, sizeof addr))
			break;		/* Success */
	}

	if (!he->h_addr_list[i])
		return "Failed to connect";

	if (!(fp = fopen(out, "w+")))
		err(1, "fetch fopen(%s)", out);

	why = ssl ?
		fetch_secure(sfd, host, msg, fp) :
		fetch_plain(sfd, msg, fp);

	if (fclose(fp))
		err(1, "fetch flose(%s)", out);

	return why;
}

int
bind_init()
{
	char buf[4096];
	FILE *fp;
	int i;

	if (!(fp = fopen(pathbinds, "r")))
		return 0;		/* Ignore error, file might not exist */

	for (i=0; fgets(buf, sizeof buf, fp); i++)
		binds[buf[0]-'A'] = strdup(trim(buf+1));

	if (fclose(fp))
		err(1, "bind_init flose %s", pathbinds);

	return i;
}

void
bind_set(char bind, char *str)
{
	int i = bind-'A';
	FILE *fp;

	if (binds[i])
		free(binds[i]);

	binds[i] = strdup(str);

	if (!(fp = fopen(pathbinds, "w")))
		err(1, "bind save fopen %s", pathbinds);

	for (i=0; i<LENGTH(binds); i++)
		if (binds[i])
			fprintf(fp, "%c\t%s\n", i+'A', binds[i]);

	if (fclose(fp))
		err(1, "bind save fclose %s", pathbinds);
}

char *
bind_get(char bind)
{
	return binds[bind-'A'];
}

char *
cache_path(int index)
{
	static char buf[4096];
	snprintf(buf, sizeof buf, "%s/%u", pathcache, index);
	return buf;
}

Why
cache_add(char *key, char *src)
{
	static int age=0;
	Why why;
	int i, min=-1, oldest=0;
	char *dst;

	/* Find empty spot or oldest entry */
	for (i=0; i<LENGTH(caches); i++) {
		if (!caches[i].key)
			break;			/* Found empty */

		if (!strcmp(caches[i].key, key)) {
			caches[i].age = age++;
			return 0;		/* Already cached */
		}

		if (caches[i].age < min) {
			min = caches[i].age;
			oldest = i;
		}
	}

	/* No empty spots, use oldest */
	if (i == LENGTH(caches)) {
		i = oldest;
		free(caches[i].key);
	}

	caches[i].key = strdup(key);
	caches[i].age = age++;

	dst = cache_path(i);

	if ((why = cp(src, dst)))
		return why;

	return 0;
}

char *
cache_get(char *key)
{
	int i;

	for (i=0; i<LENGTH(caches); i++)
		if (caches[i].key && !strcmp(caches[i].key, key))
			return cache_path(i);

	return 0;
}

void
link_clear()
{
	while (linksn)
		free(links[--linksn]);
}

char *
link_get(int i)
{
	return i >= linksn || i < 0 ? 0 : links[i];
}

int
link_store(char *uri)
{
	if (linksn+1 >= linkscap) {
		linkscap = linkscap ? linkscap*2 : 32;
		links = realloc(links, sizeof(char *) * linkscap);
		if (!links)
			err(1, "link_store realloc %s %u", uri, linkscap);
	}
	links[linksn++] = strdup(uri);
	return linksn -1;
}

char *
uri_protocolstr(int protocol)
{
	switch (protocol) {
	case LOCAL:	return "file";
	case HTTP:	return "http";
	case HTTPS:	return "https";
	case GEMINI:	return "gemini";
	case GOPHER:	return "gopher";
	}
	return 0;
}

int
uri_protocol(char *uri)
{
	assert(uri);
	if (starts(uri, "file://"))	return LOCAL;
	if (starts(uri, "http://"))	return HTTP;
	if (starts(uri, "https://"))	return HTTPS;
	if (starts(uri, "gemini://"))	return GEMINI;
	if (starts(uri, "gopher://"))	return GOPHER;
	return 0;
}

char *
uri_host(char *uri)
{
	static char buf[4096];
	char *beg, *end;
	size_t n;

	assert(uri);

	beg = (beg = strstr(uri, "://")) ? beg+3 : uri;	/* Skip protocol */
	n = strlen(beg);

	if (!n)
		return 0;

	if (!(end = strpbrk(beg, ":/")))
		end = beg + n;

	n = end - beg;

	if (!n)
		return 0;

	buf[n] = 0;

	return memcpy(buf, beg, n);
}

int
uri_port(char *uri)
{
	char *beg;
	assert(uri);
	beg = (beg = strstr(uri, "://")) ? beg+3 : uri;	/* Skip protocol */
	return (beg = strchr(beg, ':')) ? atoi(beg+1) : 0;
}

char *
uri_path(char *uri)
{
	static char buf[4096];
	char *beg;
	assert(uri);
	beg = (beg = strstr(uri, "://")) ? beg+3 : uri;	/* Skip protocol */
	beg = strchr(beg, '/');
	return beg ? strcpy(buf, beg) : "/";
}

char *
uri_normalize(char *link)
{
	static char buf[4096];
	int protocol, port;
	char *base, *host, *path, *prefix, *pt, *tmp;

	base = link_get(0);

	if (!base)
		base = "";

	if (link[0] == '/') {	/* Link is an absolute path */
		protocol = uri_protocol(base);
		port = uri_port(base);
		host = uri_host(base);
		path = link;
	} else if (strstr(link, "://")) {	/* Link is a full URI */
		protocol = uri_protocol(link);
		port = uri_port(link);
		host = uri_host(link);
		path = uri_path(link);
	} else {	/* Link is a relative path */
		protocol = uri_protocol(base);
		port = uri_port(base);
		host = uri_host(base);
		path = uri_path(base);

		snprintf(buf, sizeof buf, "%s", path);

		if ((pt = strrchr(buf, '/')))
			*(pt+1) = 0;

		strcat(buf, link);	/* TODO(irek): Possible overflow? */
		path = uri_path(buf);
	}

	if (!host)
		host = "";

	if (!protocol)
		protocol = port;

	prefix = uri_protocolstr(protocol);

	/* NOTE(irek): Protocol, and by extension the prefix, should
	 * always be defined.  Without it it's not possible for the
	 * link to be normalized.  Usually protocol is present thanks
	 * to base URI but it's still possible that first link typed
	 * by user will not have it and there will be no port to guess
	 * the protocol. */
	if (!prefix)
		return 0;

	if (!port)
		port = protocol;

	/* Resolve "//" "./" and "../" */
	while ((pt = strstr(path, "//")))
		path = pt + 1;

	while ((pt = strstr(path, "../"))) {
		if (pt == path)
			break;

		tmp = pt-1;
		if (*tmp == '/')
			tmp--;

		while (tmp > path && *tmp != '/')
			tmp--;

		skip(tmp, (pt-tmp) + 2);
	}

	pt = path;
	while ((pt = strstr(pt, "./")))
		skip(pt, 2);

	snprintf(buf, sizeof buf, "%s://%s:%d%s", prefix, host, port, path);
	return buf;
}

void
undo_add(char *uri)
{
	static const int LIMIT = 10*1024;
	static char date[32], *buf=0;
	char *mode;
	time_t timestamp;
	int n;
	struct tm *tm;
	FILE *fp;

	/* Avoid duplications */
	if (!strcmp(undos[undo_last % UNDOSN], uri))
		return;

	strcpy(undos[++undo_last % UNDOSN], uri);
	undos[(undo_last+1) % UNDOSN][0] = 0;

	if (starts(uri_path(uri), envhome))
		return;

	if (!buf && !(buf = malloc(LIMIT)))
		err(1, "undo save malloc");

	mode = access(pathhistory, F_OK) ? "w+" : "r+";
	if (!(fp = fopen(pathhistory, mode)))
		err(1, "undo save fopen %s", pathhistory);

	if (fseek(fp, 0, SEEK_SET) == -1)
		err(1, "undo save fseek %s", pathhistory);

	if ((timestamp = time(0)) == (time_t) -1)
		err(1, "undo save time");

	if (!(tm = localtime(&timestamp)))
		err(1, "undo save localtime");

	strftime(date, sizeof date, "%Y.%m.%d", tm);
	n = snprintf(buf, LIMIT, "=> %s\t%s %s\n", uri, date, uri);
	n += fread(buf+n, 1, LIMIT-n, fp);

	if (fseek(fp, 0, SEEK_SET) == -1)
		err(1, "undo save fseek %s", pathhistory);

	fwrite(buf, 1, n, fp);

	if (fclose(fp))
		err(1, "undo save fclose %s", pathhistory);
}

char *
undo_goto(int n)
{
	int i=n, d = n>0 ? -1 : +1;

	for (; undo_last && i; i+=d, undo_last-=d)
		if (!undos[(undo_last -d) % UNDOSN][0])
			break;

	return i == n ? 0 : undos[undo_last % UNDOSN];
}

Mime
mime_path(char *path)
{
	char *extension;

	extension = strrchr(path, '.');
	if (!extension) return MIME_TEXT;
	if (!strcasecmp(extension, ".txt"))  return MIME_TEXT;
	if (!strcasecmp(extension, ".gph"))  return MIME_GPH;
	if (!strcasecmp(extension, ".gmi"))  return MIME_GMI;
	if (!strcasecmp(extension, ".exe"))  return MIME_BINARY;
	if (!strcasecmp(extension, ".bin"))  return MIME_BINARY;
	return MIME_TEXT;
}

Mime
mime_header(char *str)
{
	if (starts(str, "text/gemini")) return MIME_GMI;
	if (starts(str, "text/")) return MIME_TEXT;
	if (starts(str, "application/gopher-menu")) return MIME_GPH;
	return MIME_BINARY;
}

int
main(int argc, char **argv)
{
	int opt;
	char buf[4096], *prompt=0, *env;
	struct sigaction sa;

	sa.sa_handler = onsignal;
	sigaction(SIGTERM, &sa, 0);
	sigaction(SIGINT, &sa, 0);

	envstr("PAGER",      &envpager);
	envstr("YUPAPAGER",  &envpager);
	envstr("YUPAHTML",   &envhtml);
	envint("YUPAMARGIN", &envmargin);
	envint("YUPAWIDTH",  &envwidth);

	if ((env = getenv("YUPAHOME")))
		envhome = env;

	envhome = strdup(resolvepath(envhome));
	if (setenv("YUPAHOME", envhome, 1))
		err(1, "setenv(YUPAHOME)");

	mkdir(envhome, 0755);

	envsession = startsession();
	if (setenv("YUPASESSION", envsession, 1))
		err(1, "setenv(YUPASESSION)");

	pathlock    = strdup(join(envsession, "/.lock"));
	pathres     = strdup(join(envsession, "/res"));
	pathout     = strdup(join(envsession, "/out"));
	pathlinks   = strdup(join(envsession, "/links"));
	pathcmd     = strdup(join(envsession, "/cmd"));
	pathcache   = strdup(join(envsession, "/cache"));
	pathbinds   = strdup(join(envhome, "/binds"));
	pathhistory = strdup(join(envhome, "/history.gmi"));
	pathbook    = strdup(join(envhome, "/book.gmi"));

	if (envmargin < 0)
		errx(0, "YUPAMARGIN has to be >= 0");

	if (envwidth < envmargin)
		errx(0, "YUPAMARGIN has to be > YUPAMARGIN");

	while ((opt = getopt(argc, argv, "vh")) != -1)
		switch (opt) {
		case 'v':
			puts(NAME" "VERSION" by <"AUTHOR">");
			end(0);
		case 'h':
			usage(argv[0]);
			end(0);
		default:
			usage(argv[0]);
			end(1);
		}

	mkdir(pathcache, 0755);

	/* Define default binds when there are none */
	if (bind_init() == 0) {
		bind_set('A', "! echo '=>' $YUPAURI >> $YUPAHOME/book.gmi");
		bind_set('B', "% echo file://$YUPAHOME/book.gmi");
		bind_set('H', "% echo file://$YUPAHOME/history.gmi");
		bind_set('V', "gopher://gopher.floodgap.com/7/v2/vs");
	}

	if (argc - optind > 0)
		prompt = argv[argc-optind];

	if (prompt) {
		onprompt(prompt);
	} else if (!access(pathhistory, F_OK)) {
		env = envpager;
		envpager = "head";
		onprompt(join("file://", pathhistory));
		envpager = env;
	}

	while (fgets(buf, sizeof buf, stdin))
		onprompt(buf);

	end(0);
}
