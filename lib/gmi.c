#include <assert.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <string.h>
#include "gmi.h"
#include "le.h"
#include "net.h"
#include "uri.h"
#include "util.h"

static void
format(FILE *src, FILE *dst)
{
	char buf[4096], *bp;
	size_t len;
	int link=0, partial=0;
	assert(src);
	assert(dst);
	rewind(src);
	while ((bp = fgets(buf, sizeof(buf), src))) {
		len = strlen(bp);
		if (partial) {
			fputs(bp, dst);
			// TODO(irek): Duplicated.
			partial = len == 0 || bp[len-1] != '\n';
			continue;
		}
		partial = len == 0 || bp[len-1] != '\n';
		if (len > 1 && !strncmp(bp, "=>", 2)) {
			// Link
			fprintf(dst, "(%d)\t", ++link);
		}
		fputs(bp, dst);
	}
}

FILE *
gmi_req(FILE *raw, FILE *fmt, char *uri)
{
	char buf[4096], *tmp, *host;
	int port, sfd, siz;
	FILE *show;
        SSL_CTX *ctx;
        SSL *ssl=0;
	assert(raw);
	assert(fmt);
	assert(uri);
	host = uri_host(uri);
	port = uri_port(uri);
	if (!port) {
		port = URI_GEMINI;
	}
	if (!(ctx = SSL_CTX_new(TLS_client_method()))) {
		WARN("SSL_CTX_new");
		return 0;
	}
	if (!(sfd = tcp(host, port))) {
		printf("Request '%s' failed\n", uri);
		return 0;
	}
        if ((ssl = SSL_new(ctx)) == 0) {
		WARN("SSL_new");
		return 0;
	}
        if (!SSL_set_tlsext_host_name(ssl, host)) {
		WARN("SSL_set_tlsext");
		return 0;
	}
        if (!SSL_set_fd(ssl, sfd)) {
		WARN("SSL_set_fd");
		return 0;
	}
        if (SSL_connect(ssl) < 1) {
		WARN("SSL_connect");
		return 0;
	}
	tmp = JOIN(uri, "\r\n");
        if (SSL_write(ssl, tmp, strlen(tmp)) < 1) {
		WARN("SSL_write");
		return 0;
	}
	while ((siz = SSL_read(ssl, buf, sizeof(buf)-1))) {
		buf[siz] = 0;
		fputs(buf, raw);
	}
	SSL_free(ssl);
	show = fmt;
	if (show == fmt) {
		format(raw, fmt);
	}
	return show;
}

char *
gmi_uri(FILE *body, int index)
{
	static char uri[URI_SIZ];
	char buf[4096], *bp;
	size_t len;
	int partial=0;
	assert(body);
	assert(index > 0);
	while ((bp = fgets(buf, sizeof(buf), body))) {
		len = strlen(bp);
		if (partial) {
			// TODO(irek): Duplicated.
			partial = len == 0 || bp[len-1] != '\n';
			continue;
		}
		partial = len == 0 || bp[len-1] != '\n';
		if (len > 1 && !strncmp(bp, "=>", 2)) {
			index--;
		}
		if (index == 0) {
			bp += 2;
			while (*bp <= ' ') bp++;
			bp[strcspn(bp, "\t\n ")] = 0;
			strcpy(uri, bp);
			return uri;
		}
	}
	return 0;
}
