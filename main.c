/* Yupa v0.1 by irek@gabr.pl */

#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "arg.h"
#include "uri.h"
#include "gph.c"

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
	ssize_t ssiz;
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
	while ((ssiz = recv(sfd, buf, sizeof(buf), 0)) > 0) {
		if (fwrite(buf, 1, ssiz, stdout) != (size_t)ssiz) {
			die("onuri fwrite:");
		}
	}
	if (ssiz < 0) {
		die("recv:");
	}
	if (close(sfd)) {
		die("close(%d):", sfd);
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
	if (argc) {
		onuri(argv[0]);
	}
	return 0;
}
