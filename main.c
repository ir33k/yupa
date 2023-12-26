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

#define LOG_IMPLEMENTATION
#include "log.h"

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

/* Print usage help message and die. */
static void
usage(void)
{
	DIE("$ %s [-h] [uri]\n"
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
 * PORT.  Return socket file descriptor or 0 on error. */
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
		WARN("socket %s %d:", host, port);
		return 0;
	}
	if ((he = gethostbyname(host)) == 0) {
		WARN("gethostbyname %s %d:", host, port);
		return 0;
	}
	for (i = 0; he->h_addr_list[i]; i++) {
		memcpy(&addr.sin_addr.s_addr, he->h_addr_list[i], sizeof(in_addr_t));
		if (connect(sfd, (struct sockaddr*)&addr, sizeof(addr))) {
			continue;
		}
		return sfd;	/* Success */
	}
	WARN("failed to connect with %s %d:", host, port);
	return 0;
}

/*
 * TODO(irek): I dislike this function.  Merging it with onuri?
 */
/* Open connection to server under HOST with PORT and optional PATH.
 * Return socket file descriptor on success that is ready to read
 * server response.  Return 0 on error. */
static int
req(char *host, int port, char *path)
{
	int sfd;
	assert(host);
	assert(port > 0);
	if ((sfd = tcp(host, port)) < 0) {
		WARN("tcp %s %d:", host, port);
		return 0;
	}
	if (path) {
		if (send(sfd, path, strlen(path), 0) == -1) {
			WARN("send %s %d %s:", host, port, path);
			return 0;
		}
	}
	if (send(sfd, "\r\n", 2, 0) == -1) {
		WARN("send %s %d %s:", host, port, path);
		return 0;
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
		WARN("Only 'gopher://' protocol is supported and assumed by default");
		return;
	}
	if (path && path[1]) {
		item = path[1];
		(void)item;
		path += 2;
	}
	/* TODO(irek): Figure out error handling. */
	if ((sfd = req(host, port, path)) == 0) {
		WARN("");
		return;
	}
	if (!(fp = fopen(s_tab.body, "w"))) {
		ERR("fopen(%s):", s_tab.body);
	}
	while ((ssiz = recv(sfd, buf, sizeof(buf), 0)) > 0) {
		if (fwrite(buf, 1, ssiz, fp) != (size_t)ssiz) {
			ERR("fwrite:");
		}
	}
	if (ssiz < 0) {
		ERR("recv:");
	}
	if (fclose(fp) == EOF) {
		ERR("fclose(%s):", s_tab.body);
	}
	if (close(sfd)) {
		ERR("close(%d):", sfd);
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
			ERR("fputs:");
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
			INFO("LINK %d", atoi(buf));
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
		ERR("open(%s):", dst);
	}
	if (close(fd)) {
		ERR("close(%s):", dst);
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
