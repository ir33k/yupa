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

//
static void
format(FILE *src, FILE *dst)
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
}

enum net_res
gmi_req(FILE *raw, FILE *fmt, char *uri)
{
	char buf[4096], head[1024], *tmp, *host;
	int port, sfd, sz;
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
	SSL_read(ssl, head, sizeof(head));
	switch (head[0]) {
	case GMI_QUERY:
		SSL_free(ssl);
		return NET_URI; // TODO(irek)
	case GMI_REDIRECT:
		// TODO(irek): Redirection can navigate to other
		// protocols and because of that this logic should be
		// handled in main program.  I have to redesign entire
		// flow to make it possible.
		SSL_free(ssl);
		return NET_URI;
	}
	while ((sz = SSL_read(ssl, buf, sizeof(buf)-1))) {
		buf[sz] = 0;
		fputs(buf, raw);
	}
	SSL_free(ssl);
	// TODO(irek): At this point I should have mime type parser.
	if (strncmp(head+3, "text/", 5)) {
		return NET_BIN;
	}
	if (strncmp(head+3, "text/gemini", 11)) {
		format(raw, fmt);
		return NET_FMT;
	}
	return NET_RAW;
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
	// format() and this function.  Could be extracted to keep
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
