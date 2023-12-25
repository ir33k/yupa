/* Yupa v0.1 by irek@gabr.pl */

#include <assert.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include "arg.h"
#include "uri.h"
#include "gph.c"

/* BUFSIZ is used as default buffer size in many places and it has to
 * be big enough to hold an single URI string. */
_Static_assert(BUFSIZ > URI_SIZ, "BUFSIZ is too small");

enum cmd {
	CMD_NUL = 0,
	CMD_URI,
	CMD_LINK,
	CMD_RELOAD,
	CMD_QUIT
};

struct tab {
	enum uri_protocol protocol;
	char    uri[URI_SIZ];
	char    body[FILENAME_MAX];
	char    pager[BUFSIZ];
};

static struct tab s_tab = {0};
char *argv0;                    /* First program arg, for arg.h */

/* Print FMT (printf(3)) message to stderr and exit with code 1.
 * If FMT ends with ':' then append errno error. */
static void
die(const char *fmt, ...)
{
	va_list ap;
	assert(fmt);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(0);
	} else {
		fputc('\n', stderr);
	}
	exit(1);
}

/* Print usage help message and die. */
static void
usage(void)
{
	die("$ %s [-h] [uri]\n"
	    "\n"
	    "	-h	Print help message.\n"
	    "	[uri]	URI to open.\n"
	    "\n", argv0);
}

/* Return non 0 value when STR contains only digits. */
static int
isnum(char *str)
{
	for (; *str; str++) {
		if (*str < '0' || *str > '9') {
			return 0;
		}
	}
	return 1;
}

/* Establish AF_INET internet SOCK_STREAM stream connection to HOST of
 * PORT.  Return socket file descriptor.  Negative value on error. */
static int
tcp(char *host, int port)
{
	int i, sfd;
	struct hostent *he;
	struct sockaddr_in addr;
	assert(host);
	assert(port > 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if ((sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		return -1;
	}
	if ((he = gethostbyname(host)) == 0) {
		return -2;
	}
	for (i=0; he->h_addr_list[i]; i++) {
		memcpy(&addr.sin_addr.s_addr, he->h_addr_list[i], sizeof(in_addr_t));
		if (connect(sfd, (struct sockaddr*)&addr, sizeof(addr))) {
			continue;
		}
		return sfd;	/* Success */
	}
	return -3;
}

/* TODO(irek): I dislike this function.  Merging it with onuri? */
/* Open connection to server under HOST with PORT and optional PATH.
 * Return socket file descriptor on success that is ready to read
 * server response.  Return negative value on error. */
static int
req(char *host, int port, char *path)
{
	int sfd;
	assert(host);
	assert(port > 0);
	if ((sfd = tcp(host, port)) < 0) {
		return -1;
	}
	if (path) {
		if (send(sfd, path, strlen(path), 0) == -1) {
			return -2;
		}
	}
	if (send(sfd, "\r\n", 2, 0) == -1) {
		return -3;
	}
	return sfd;
}

static void
onuri(char *uri)
{
	enum uri_protocol protocol;
	int sfd, port;
	char buf[BUFSIZ], *host, *path, item;
	FILE *fp;
	ssize_t ssiz;
	assert(strlen(uri) <= URI_SIZ);
	protocol = uri_protocol(uri);
	host = uri_host(uri);
	port = uri_port(uri);
	path = uri_path(uri);
	if (!port) {
		port = GPH_PORT;
	}
	if (!protocol) {
		protocol = URI_PROTOCOL_GOPHER;
	}
	if (protocol != URI_PROTOCOL_GOPHER) {
		die("Only 'gopher://' protocol is supported and assumed by default");
	}
	if (path && path[1]) {
		item = path[1];
		(void)item;
		path += 2;
	}
	/* TODO(irek): Figure out error handling. */
	switch ((sfd = req(host, port, path))) {
	case -1:
		die("onuri tcp(%s, %d): Fail to connect", host, port);
		break;
	case -2:
		die("onuri send(path):");
		break;
	case -3:
		die("onuri send():");
		break;
	}
	if (!(fp = fopen(s_tab.body, "w"))) {
		die("onuri fopen(%s):", s_tab.body);
	}
	while ((ssiz = recv(sfd, buf, sizeof(buf), 0)) > 0) {
		if (fwrite(buf, 1, ssiz, fp) != (size_t)ssiz) {
			die("onuri fwrite:");
		}
	}
	if (fclose(fp) == EOF) {
		die("onuri fclose(%s):", s_tab.body);
	}
	if (ssiz < 0) {
		die("recv:");
	}
	if (close(sfd)) {
		die("close(%d):", sfd);
	}
	s_tab.protocol = protocol;
	strcpy(s_tab.uri, uri);
	system(s_tab.pager);
}

static enum cmd
cmd(char *buf, size_t siz)
{
	if (isnum(buf))         return CMD_LINK;
#define CMD_IS(_str) strlen(_str) == siz && !strcasecmp(_str, buf)
	if (CMD_IS("q"))        return CMD_QUIT;
	if (CMD_IS("quit"))     return CMD_QUIT;
	if (CMD_IS("exit"))     return CMD_QUIT;
	if (CMD_IS("0"))        return CMD_RELOAD;
	if (CMD_IS("r"))        return CMD_RELOAD;
	if (CMD_IS("reload"))   return CMD_RELOAD;
	if (CMD_IS("refresh"))  return CMD_RELOAD;
	return CMD_URI;
}

static void
run(void)
{
	char buf[BUFSIZ];
	size_t len;
	while (1) {
		if (fputs("yupa> ", stdout) == EOF) {
			die("run fputs:");
		}
		if (!fgets(buf, sizeof(buf), stdin)) {
			continue;
		}
		len = strlen(buf) - 1;
		buf[len] = 0;
		switch (cmd(buf, len)) {
		case CMD_URI:
			onuri(buf);
			break;
		case CMD_LINK:
			printf(">>> LINK %d\n", atoi(buf));
			break;
		case CMD_RELOAD:
			onuri(s_tab.uri);
			break;
		case CMD_QUIT:
			exit(0);
			break;
		case CMD_NUL:
		default:
			break;
		}
	}
}

static char *
strrand(int len)
{
	static int seed = 0;
	static const char *allow =
		"ABCDEFGHIJKLMNOPRSTUWXYZ"
		"abcdefghijklmnoprstuwxyz"
		"0123456789";
	size_t limit = strlen(allow);
	static char str[32];
	assert(len < 32);
	srand(time(0) + seed++);
	str[len] = 0;
	while (len--) {
		str[len] = allow[rand() % limit];
	}
	return str;
}

static void
tmpf(char *dst)
{
	static const char *prefix = "/tmp/yupa";
	int fd;
	do {
		sprintf(dst, "%s%s", prefix, strrand(6));
	} while (!access(dst, F_OK));
	if ((fd = open(dst, O_RDWR | O_CREAT | O_EXCL)) == -1) {
		die("tmpf open(%s)", dst);
	}
	if (close(fd)) {
		die("tmpf close(%s)", dst);
	}
}

int
main(int argc, char *argv[])
{
	ARGBEGIN {
	case 'h':
	default:
		usage();
	} ARGEND
	tmpf(s_tab.body);
	sprintf(s_tab.pager, "cat %s", s_tab.body);
	if (argc) {
		onuri(argv[0]);
	}
	run();
	return 0;
}
