#define NAME	"yupa"
#define VERSION	"v5.3"
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
#include <time.h>
#include <unistd.h>

#include "main.h"
#include "embed.h"
#include "gmi.h"
#include "gph.h"

#define CRLF		"\r\n"		/* Terminates request messages */
#define SESSIONSN	16		/* Arbitrary limit to avoid insanity */
#define BINDSN		('Z'-'A'+1)	/* Inclusive range from A to Z */
#define UNDOSN		32

enum {					/* Default ports */
	LOCAL	= 4,
	HTTP	= 80,
	HTTPS	= 443,
	GEMINI	= 1965,
	GOPHER	= 70,
};

typedef char		Undo[4096];
typedef struct cache	Cache;

struct cache {
	char*	key;
	int	age;
};

static void	usage		(char *argv0);
static void	envstr		(char *name, char **env);
static void	envint		(char *name, int *env);
static Err	loadpage	(char *link);
static void	onprompt	(char*);
static char*	oncmd		(char*);
static void	end		(int) __attribute__((noreturn));
static void	onsignal	(int);
static char*	startsession	();
static void	save		(char *name, const char *str);
static char*	join		(char*, char*);
static char*	resolvepath	(char*);
static Err	fcp		(FILE *src, FILE *dst);
static Err	cp		(char *src, char *dst);
static void	skip		(char *str, unsigned n);

/* Fetch */
static Err	fetch_secure	(int sfd, char *host, char *msg, FILE *out);
static Err	fetch_plain	(int sfd, char *msg, FILE *out);
static Err	fetch		(char *host, int port, int ssl, char *msg, char *out);

/* Binds are characters in range from A to Z that hold any string value */
static void	bind_init	();
static void	bind_set	(char bind, char *str);
static char*	bind_get	(char bind);

/* Cache some number of files */
static char*	cache_path	(int index);
static Err	cache_add	(char *key, char *path);
static char*	cache_get	(char *key);
static void	cache_clear	();

/* Links storage */
static void	link_clear	();
static char*	link_get	(int i);

/* Extract parts of URI string */
static char*	uri_protocolstr	(int protocol);
static int	uri_protocol	(char *uri);
static char*	uri_host	(char *uri);
static int	uri_port	(char *uri);
static char*	uri_path	(char *uri);
static char*	uri_normalize	(char *link, char *base_uri);

/* Browsing undo history */
static void	undo_add	(char *uri);
static char*	undo_goto	(int offset);

/* Mime */
static Mime	mime_path	(char *path);
static Mime	mime_header	(char *str);

static int	silent		= 0;
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
static char*	envimage	= "xdg-open";
static char*	envvideo	= "xdg-open";
static char*	envaudio	= "xdg-open";
static char*	envpdf		= "xdg-open";
static char*	envhtml		= "xdg-open";
int	envmargin	= 4;
int	envwidth	= 76;

static char*	pathlock;
static char*	pathres;
static char*	pathout;
static char*	pathuri;
static char*	pathinfo;
static char*	pathcmd;
static char*	pathcache;
static char*	pathbinds;
static char*	pathhistory;

void
usage(char *argv0)
{
	printf("usage: %s [options] [URI/input]\n"
	       "\n"
	       "options:\n"
	       "	-v		Print program version\n"
	       "	-h		Print this help message\n"
	       "\n"
	       "envs:\n"
	       "	YUPAHOME	Absolute path to user data (%s)\n"
	       "	YUPASESSION	Runtime path to session dir (%s)\n"
	       "	YUPAPAGER	Overwrites $PAGER value (%s)\n"
	       "	YUPAIMAGE	Command to display images (%s)\n"
	       "	YUPAVIDEO	Command to play videos (%s)\n"
	       "	YUPAAUDIO	Command to play audio (%s)\n"
	       "	YUPAPDF		Command to open PDFs (%s)\n"
	       "	YUPAHTML	Command to open websites (%s)\n"
	       "	YUPAMARGIN	Left margin (%d)\n"
	       "	YUPAWIDTH	Max width (%d)\n",
	       argv0,
	       envhome, envsession,
	       envpager, envimage, envvideo, envaudio, envpdf, envhtml,
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

Err
loadpage(char *link)
{
	Err why;
	char uri[4096], buf[4096];
	char *host, *path, *cache, *search, *pt, *cmd, *header;
	int protocol, port, ssl=0;
	Mime mime;
	int n;
	FILE *fp, *res, *out;

	if (!link)
		return "No link";

	pt = uri_normalize(link, link_get(0));
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

	if (!(fp = fopen(pathuri, "w")))
		err(1, "fopen(%s)", pathuri);

	fprintf(fp, "%s", uri);

	if (fclose(fp))
		err(1, "flose(%s)", pathuri);

	if (!(out = fopen(pathout, "w")))
		err(1, "fopen(%s)", pathout);

	cmd = 0;
	switch (mime) {
	default:
	case 0:
		why = "Unsupported mime file type";
		break;
	case MIME_BINARY:
		why = "Binary mime file type is unsupported";
		break;
	case MIME_GPH:
		gph_print(res, out);
		cmd = envpager;
		break;
	case MIME_GMI:
		gmi_print(res, out);
		cmd = envpager;
		break;
	case MIME_TEXT:  if (!(why = fcp(res, out))) cmd = envpager; break;
	case MIME_HTML:  if (!(why = fcp(res, out))) cmd = envhtml;  break;
	case MIME_IMAGE: if (!(why = fcp(res, out))) cmd = envimage; break;
	case MIME_VIDEO: if (!(why = fcp(res, out))) cmd = envvideo; break;
	case MIME_AUDIO: if (!(why = fcp(res, out))) cmd = envaudio; break;
	case MIME_PDF:   if (!(why = fcp(res, out))) cmd = envpdf;   break;
	}

	if (fclose(res))
		err(1, "flose(%s)", pathres);

	if (fclose(out))
		err(1, "flose(%s)", pathout);

	if (!silent && cmd) {
		snprintf(buf, sizeof buf, "%s %s", cmd, pathout);
		system(buf);
	}

	return why;
}

void
onprompt(char *str)
{
	Err why;
	char *link;

	if (!str || !str[0])
		return;

	if (isdigit(str[0]))
		link = link_get(atoi(str));
	else
		link = oncmd(str);

	if (!link)
		return;

	if ((why = loadpage(link)))
		printf(NAME": %s\n", why);
}

char *
oncmd(char *cmd)
{
	static char buf[4096];
	char *arg, *str=0;
	int i;
	FILE *fp;

	if (!cmd[0])
		return 0;

	/* When second character is not a whitespace (including null
	 * terminator) and it's not a digit then we can't tell if this
	 * is an Link or invalid command.  It's better to return it
	 * right away to try interpret it as a link. */
	if (cmd[1] > ' ' && !isdigit(cmd[1]))
		return cmd;

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
		end(0);
	case 'h':
		snprintf(buf, sizeof buf, "file://%s/%s",
			 envhome, "help/index.gmi");
		onprompt(buf);
		break;
	case 'b':
		i = atoi(arg);
		return undo_goto(i ? -i : -1);
	case 'f':
		i = atoi(arg);
		return undo_goto(i ? i : 1);
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
	case 'c':
		cache_clear();
		break;
	case 's':
		silent = !silent;
		printf("Silent mode %s\n", silent ? "enabled" : "disabled");
		break;
	case '$':
		system(arg);
		break;
	case '!':
		snprintf(buf, sizeof buf, "%s %s", arg, pathout);
		system(buf);
		break;
	case '|':
		snprintf(buf, sizeof buf, "<%s %s", pathout, arg);
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
	}

	return 0;
}

void
end(int code)
{
	cache_clear();
	unlink(pathlock);
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

	for (i=0; i<SESSIONSN; i++) {
		snprintf(buf, sizeof buf, "%s/%d", envhome, i);

		if (access(buf, F_OK)) {
			if (mkdir(buf, 0755))
				err(1, "startsession mkdir(%s)", buf);

			break;	/* New session */
		}

		if (access(join(buf, "/.lock"), F_OK))
			break;	/* Free session */
	}

	if (i == SESSIONSN)
		errx(1, "Exceeded maximum number of sessions %d", SESSIONSN);

	fd = creat(join(buf, "/.lock"), 0644);
	if (fd == -1)
		err(1, "startsession create(%s)", buf);

	close(fd);

	return strdup(buf);
}

void
save(char *name, const char *str)
{
	FILE *fp;
	char *path;

	path = join(envhome, name);

	if (!access(path, F_OK))
		return;

	if (!(fp = fopen(path, "w")))
		err(1, "save fopen %s", path);

	fwrite(str, 1, strlen(str), fp);

	if (fclose(fp))
		err(1, "save fclose %s", path);
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

Err
fcp(FILE *src, FILE *dst)
{
	char buf[BUFSIZ];
	size_t n;

	while ((n = fread(buf, 1, sizeof buf, src)))
		if (fwrite(buf, 1, n, dst) != n)
			return "Failed to copy files";

	return 0;
}

Err
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
	if (!str) str = "";
	int n = strlen(with);
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

Err
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

Err
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

Err
fetch(char *host, int port, int ssl, char *msg, char *out)
{
	Err why;
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

void
bind_init()
{
	char buf[4096];
	FILE *fp;

	if (!(fp = fopen(pathbinds, "r")))
		return;		/* Ignore error, file might not exist */

	while (fgets(buf, sizeof buf, fp))
		binds[buf[0]-'A'] = strdup(trim(buf+1));

	if (fclose(fp))
		err(1, "bind_init flose %s", pathbinds);
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

Err
cache_add(char *key, char *src)
{
	static int age=0;
	Err why;
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
cache_clear()
{
	int i;

	for (i=0; i<LENGTH(caches); i++)
		if (caches[i].key)
			unlink(cache_path(i));

	memset(caches, 0, sizeof caches);
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
uri_normalize(char *link, char *base)
{
	static char buf[4096];
	int protocol, port;
	char *host, *path, *prefix, *pt, *tmp;

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

	if (!path || !path[0])
		return 0;

	extension = strrchr(path, '.');
	if (!extension) return MIME_TEXT;
	if (!strcasecmp(extension, ".txt"))  return MIME_TEXT;
	if (!strcasecmp(extension, ".md"))   return MIME_TEXT;
	if (!strcasecmp(extension, ".gph"))  return MIME_GPH;
	if (!strcasecmp(extension, ".gmi"))  return MIME_GMI;
	if (!strcasecmp(extension, ".html")) return MIME_HTML;
	if (!strcasecmp(extension, ".jpg"))  return MIME_IMAGE;
	if (!strcasecmp(extension, ".jpeg")) return MIME_IMAGE;
	if (!strcasecmp(extension, ".png"))  return MIME_IMAGE;
	if (!strcasecmp(extension, ".bmp"))  return MIME_IMAGE;
	if (!strcasecmp(extension, ".gif"))  return MIME_IMAGE;
	if (!strcasecmp(extension, ".mp4"))  return MIME_VIDEO;
	if (!strcasecmp(extension, ".mov"))  return MIME_VIDEO;
	if (!strcasecmp(extension, ".avi"))  return MIME_VIDEO;
	if (!strcasecmp(extension, ".mkv"))  return MIME_VIDEO;
	if (!strcasecmp(extension, ".wav"))  return MIME_AUDIO;
	if (!strcasecmp(extension, ".mp3"))  return MIME_AUDIO;
	if (!strcasecmp(extension, ".pdf"))  return MIME_PDF;

	return MIME_BINARY;
}

Mime
mime_header(char *str)
{
	if (!str || !str[0])
		return 0;

	if (starts(str, "text/gemini"))              return MIME_GMI;
	if (starts(str, "text/html"))                return MIME_HTML;
	if (starts(str, "text/"))                    return MIME_TEXT;
	if (starts(str, "image/"))                   return MIME_IMAGE;
	if (starts(str, "video/"))                   return MIME_VIDEO;
	if (starts(str, "audio/"))                   return MIME_AUDIO;
	if (starts(str, "application/pdf"))          return MIME_PDF;
	if (starts(str, "application/octet-stream")) return MIME_BINARY;
	if (starts(str, "application/gopher-menu"))  return MIME_GPH;

	return 0;
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
	envstr("YUPAIMAGE",  &envimage);
	envstr("YUPAVIDEO",  &envvideo);
	envstr("YUPAAUDIO",  &envaudio);
	envstr("YUPAPDF",    &envpdf);
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
	pathuri     = strdup(join(envsession, "/uri"));
	pathinfo    = strdup(join(envsession, "/info"));
	pathcmd     = strdup(join(envsession, "/cmd"));
	pathcache   = strdup(join(envsession, "/cache"));
	pathbinds   = strdup(join(envhome, "/binds"));
	pathhistory = strdup(join(envhome, "/history.gmi"));

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
	mkdir(join(envhome, "/help"), 0755);

	save("/binds",            embed_binds);
	save("/help/binds.gmi",   embed_help_binds_gmi);
	save("/help/cache.gmi",   embed_help_cache_gmi);
	save("/help/envs.gmi",    embed_help_envs_gmi);
	save("/help/history.gmi", embed_help_history_gmi);
	save("/help/index.gmi",   embed_help_index_gmi);
	save("/help/links.gmi",   embed_help_links_gmi);
	save("/help/session.gmi", embed_help_session_gmi);
	save("/help/shell.gmi",   embed_help_shell_gmi);
	save("/help/support.gmi", embed_help_support_gmi);

	bind_init();

	if (argc - optind > 0)
		prompt = argv[argc-optind];

	if (prompt) {
		onprompt(prompt);
	} else if (!access(pathhistory, F_OK)) {
		silent = 1;
		onprompt(join("file://", pathhistory));
		onprompt("$ less -XI +Rq $YUPASESSION/out");
		silent = 0;
	}

	while (1) {
		printf(NAME"> ");
		if (!fgets(buf, sizeof buf, stdin))
			break;

		onprompt(trim(buf));
	}

	end(0);
}
