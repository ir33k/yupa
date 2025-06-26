#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include "util.h"
#include "fetch.h"

#define CRLF "\r\n"

static why_t secure(int sfd, char *host, char *msg, FILE *out);
static why_t plain(int sfd, char *msg, FILE *out);

why_t
secure(int sfd, char *host, char *msg, FILE *out)
{
	static SSL_CTX *ctx=0;
	static SSL *ssl=0;

	char buf[4096];
	size_t n;

	assert(sfd);
	assert(host);
	assert(msg);

	/* NOTE(irek): This is an easy way of avoiding memory leak.
	 * By having both variables as static I can check on every
	 * next function usage if there is something to free.  It's
	 * true that there will be an unused memory laying around
	 * after last function usage but this is not a big deal.  It's
	 * more important to avoid memory leak and this is a very
	 * simple and elegant way of doing that in C where normally it
	 * would be a mess with many return statements.  I use the
	 * same trick for sfd in fetch(). */
	SSL_CTX_free(ctx);
	ctx = 0;
	SSL_free(ssl);
	ssl=0;

	if (!(ctx = SSL_CTX_new(TLS_client_method())))
		return "Failed to create SSL context";

	if (!(ssl = SSL_new(ctx)))
		return "Failed to create SSL instance";

	if (!SSL_set_tlsext_host_name(ssl, host))
		return tellme(0, "Failed to TLS set hostname %s", host);

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

why_t
plain(int sfd, char *msg, FILE *out)
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

why_t
fetch(char *host, int port, int ssl, char *msg, FILE *out)
{
	static int sfd=-1;
	int i;
	struct hostent *he;
	struct sockaddr_in addr;

	assert(host);
	assert(port > 0);
	assert(msg);

	if (sfd >= 0 && close(sfd))
		return "Failed to close socket";

	if ((sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		return "Failed to open socket";

	if ((he = gethostbyname(host)) == 0)
		return tellme(0, "Failed to get hostname %s", host);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	for (i=0; he->h_addr_list[i]; i++) {
		memcpy(&addr.sin_addr.s_addr, he->h_addr_list[i], sizeof(in_addr_t));
		if (!connect(sfd, (struct sockaddr*)&addr, sizeof addr))
			break;	/* Success */
	}

	if (!he->h_addr_list[i])
		return tellme(0, "Failed to connect with port %d", port);

	return ssl ?
		secure(sfd, host, msg, out) :
		plain(sfd, msg, out);
}
