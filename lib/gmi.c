#include <assert.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <string.h>
#include "gmi.h"
#include "le.h"
#include "net.h"
#include "uri.h"
#include "util.h"

// // From gmix/gmit.h project source file.
// enum gmir_code {
// 	GMIR_NUL          =  0, /* Unknown status code */
// 	                        /* 1X INPUT */
// 	GMIR_INPUT_TEXT   = 10, /* Regular input, search phrase */
// 	GMIR_INPUT_PASS   = 11, /* Sensitive input, password */
// 	                        /* 2X SUCCESS */
// 	GMIR_OK           = 20, /* All good */
// 	                        /* 3X Redirection */
// 	GMIR_REDIR_TEMP   = 30, /* Temporary redirection */
// 	GMIR_REDIR_PERM   = 31, /* Permanent redirection */
// 	                        /* 4X TMP FAIL */
// 	GMIR_WARN_TEMP    = 40, /* Temporary failure */
// 	GMIR_WARN_OUT     = 41, /* Server unavailable */
// 	GMIR_WARN_CGI     = 42, /* CGI error */
// 	GMIR_WARN_PROX    = 43, /* Proxy error */
// 	GMIR_WARN_LIMIT   = 44, /* Rate limiting, you have to wait */
// 	                        /* 5X PERMANENT FAIL */
// 	GMIR_ERR_PERM     = 50, /* Permanent failure */
// 	GMIR_ERR_404      = 51, /* Not found */
// 	GMIR_ERR_GONE     = 52, /* Resource no longer available */
// 	GMIR_ERR_NOPROX   = 53, /* Proxy request refused */
// 	GMIR_ERR_BAD      = 59, /* Bad requaest */
// 	                        /* 6X CLIENT CERT */
// 	GMIR_CERT_REQU    = 60, /* Client certificate required */
// 	GMIR_CERT_UNAUTH  = 61, /* Certificate not authorised */
// 	GMIR_CERT_INVALID = 62  /* Cerfiticate not valid */
// };

enum {
	GMI_QUERY    = '1',
	GMI_REDIRECT = '3',
};

void
gmi_fmt(FILE *src, FILE *dst)
{
	char buf[4096], *bp;
	size_t len;
	int link=0, partial=0, pre=0;
	assert(src);
	assert(dst);
	rewind(src);
	// TODO(irek): I want to do a more detailed formatting of
	// Gemini output but for, now and probably for a long time,
	// formatting of just the links will be enough.
	while ((bp = fgets(buf, sizeof(buf), src))) {
		len = strlen(bp);
		if (len == 0) {
			continue;
		}
		if (partial) {
			fputs(bp, dst);
			partial = bp[len-1] != '\n';
			continue;
		}
		partial = bp[len-1] != '\n';
		if (!strncmp(bp, "```", 3)) {
			pre = !pre;
		}
		if (pre) {
			fputs(bp, dst);
			continue;
		}
		if (!strncmp(bp, "=>", 2)) {
			// Link URI
			bp += 2;
			bp += strspn(bp, " \t\n");
			len = strcspn(bp, " \t\n\0");
			fprintf(dst, "%d:\t%.*s\n", ++link, (int)len, bp);
			// Link description
			bp += len;
			bp += strspn(bp, " \t\n");
			if (*bp) {
				fprintf(dst, "\t%s", bp);
			}
			continue;
		}
		fputs(bp, dst);
	}
	if (partial) {
		fputc('\n', dst);
	}
}

enum net_res
gmi_req(FILE *raw, FILE *fmt, char *uri, char *new)
{
	char buf[4096], head[1024], *tmp, *host;
	size_t i;
	int port, sfd, sz;
        SSL_CTX *ctx;
        SSL *ssl=0;
	assert(raw);
	assert(fmt);
	assert(uri);
	assert(new);
	host = uri_host(uri);
	port = uri_port(uri);
	if (!port) {
		port = URI_GEMINI;
	}
	if (!(ctx = SSL_CTX_new(TLS_client_method()))) {
		WARN("SSL_CTX_new");
		return NET_ERR;
	}
	if (!(sfd = net_tcp(host, port))) {
		printf("Request '%s' failed\n", uri);
		return NET_ERR;
	}
        if ((ssl = SSL_new(ctx)) == 0) {
		WARN("SSL_new");
		return NET_ERR;
	}
        if (!SSL_set_tlsext_host_name(ssl, host)) {
		WARN("SSL_set_tlsext");
		return NET_ERR;
	}
        if (!SSL_set_fd(ssl, sfd)) {
		WARN("SSL_set_fd");
		return NET_ERR;
	}
        if (SSL_connect(ssl) < 1) {
		WARN("SSL_connect");
		return NET_ERR;
	}
	tmp = JOIN(uri, "\r\n");
        if (SSL_write(ssl, tmp, strlen(tmp)) < 1) {
		WARN("SSL_write");
		return NET_ERR;
	}
	// Response header.
	for (i=0; i < sizeof(head)-1; i++) {
		if (!SSL_read(ssl, head+i, 1) ||
		    head[i] == '\n') {
			head[i-1] = 0;
			break;
		}
	}
	switch (head[0]) {
	case GMI_QUERY:
		SSL_free(ssl);
		fputs("enter search query: ", stdout);
		fflush(stdout);
		fgets(buf, sizeof(buf), stdin);
		buf[strlen(buf)-1] = 0;
		if (!buf[0]) { // Empty search
			return NET_NUL;
		}
		snprintf(new, URI_SZ, "%s?%.*s", uri, URI_SZ-8, buf);
		return NET_URI;
	case GMI_REDIRECT:
		SSL_free(ssl);
		strcpy(new, head+3);
		return NET_URI;
	}
	while ((sz = SSL_read(ssl, buf, sizeof(buf)-1))) {
		buf[sz] = 0;
		fputs(buf, raw);
	}
	SSL_free(ssl);
	if (!strncmp(head+3, "text/gemini", 11)) {
		gmi_fmt(raw, fmt);
		return NET_FMT;
	}
	if (!strncmp(head+3, "text/", 5)) {
		return NET_RAW;
	}
	return NET_BIN;
}

char *
gmi_uri(FILE *body, int index)
{
	static char uri[URI_SZ];
	char buf[4096], *bp;
	size_t len;
	int partial=0, pre=0;
	assert(body);
	assert(index > 0);
	// TODO(irek): This pattern recognision is the same for
	// gmi_fmt() and this function.  Could be extracted to keep
	// loginc in sync.
	while ((bp = fgets(buf, sizeof(buf), body))) {
		len = strlen(bp);
		if (len == 0) {
			continue;
		}
		if (partial) {
			partial = bp[len-1] != '\n';
			continue;
		}
		partial = bp[len-1] != '\n';
		if (!strncmp(bp, "```", 3)) {
			pre = !pre;
		}
		if (pre) {
			continue;
		}
		if (!strncmp(bp, "=>", 2)) {
			index--;
		}
		if (index == 0) {
			bp += 2;
			bp += strspn(bp, " \t\n");
			len = MIN(sizeof(uri)-1, strcspn(bp, "\t\n "));
			strncpy(uri, bp, len);
			uri[len] = 0;
			return uri;
		}
	}
	return 0;
}
