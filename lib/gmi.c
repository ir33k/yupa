#include <assert.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <string.h>
#include "gmi.h"
#include "le.h"
#include "net.h"
#include "uri.h"
#include "util.h"

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
		fputs(buf, fmt);
	}
	SSL_free(ssl);
	show = fmt;
	return show;
}

void
gmi_fmt(FILE *body, FILE *dst)
{
	assert(body);
	assert(dst);
}

char *
gmi_uri(FILE *body, int index)
{
	assert(body);
	assert(index > 0);
	WARN("Implement");
	return 0;
}
