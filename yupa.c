#include <assert.h>
#include <err.h>
#include <getopt.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef char* why_t;	/* For errors, tell my why! */

#define CRLF "\r\n"

enum { HTTP=80, HTTPS=443, GEMINI=1965, GOPHER=70 };

enum { NODE=0,	/* For everything else, not relevant for rendering */
	/* TITLE, */ /* TODO(irek): Unsure how to handle metadata */
	/* Block elements */
	PARAGRAPH, BR, H1, H2, H3, H4, H5, H6,
	BLOCKQUOTE, PRE, HR, FORM,
	UL, OL, LI,	/* Lists */
	DD, DL, DETAILS,
	TABLE, TR, TD, THEAD, TH,
	/* Span elements */
	TEXT, SPAN, BOLD, EM, IDIOMATIC, UNDERLINE, CODE,
	ANCHOR,	/* The <a>, I'm not using LINK because there is <link> */
	BUTTON, INPUT, SELECT };

enum { ATTR=0, ID, CLASS, STYLE, TITLE, HREF, SRC, ALT, DATA };

/* Dynamic PT data with N elements of ITEM size and available CAPACITY */
struct dynamic {
	void *pt;
	size_t item, capacity;
	unsigned n;
};

struct node {
	unsigned next, child, parent, attr;
	int type;
	char *name, *value;
};

struct attr {
	unsigned next;
	int type;
	char *name, *value;
};

static void str_tolower(char *str);
static int str_starts_with(char *str, char *prefix);
static void dynamic_init(struct dynamic *, size_t item, unsigned n);
static void dynamic_add(struct dynamic *, unsigned n);
static void dynamic_append(struct dynamic *, void *buf, unsigned n);
static void dynamic_clear(struct dynamic *);
static int url_protocol(char *url);
static char *url_host(char *url);
static int url_port(char *url);
static char *url_path(char *url);
static why_t net_fetch_secure(int sfd, char *host, char *msg);
static why_t net_fetch_plain(int sfd, char *msg);
static why_t net_fetch(char *host, int port, int ssl, char *msg);
static void parse_response(int protocol);
static void run(char *url);

static const char *help = "usage: %s URL";
static struct dynamic response, nodes, attrs;

void
str_tolower(char *str)
{
	for (; str && *str; str++)
		*str = 0x20 | (*str);
}

int
str_starts_with(char *str, char *prefix)
{
	unsigned sn, pn;

	sn = str ? strlen(str) : 0;
	pn = prefix ? strlen(prefix) : 0;

	if (prefix == 0 || sn < pn)
		return 0;

	return !strncmp(str, prefix, pn);
}

void
dynamic_init(struct dynamic *d, size_t item, unsigned n)
{
	memset(d, 0, sizeof(*d));
	d->item = item;
	dynamic_add(d, n);
}

void
dynamic_add(struct dynamic *d, unsigned n)
{
	d->capacity += n;
	d->pt = realloc(d->pt, d->capacity * d->item);

	if (!d->pt)
		err(1, "dynamic realloc");
}

void
dynamic_append(struct dynamic *d, void *buf, unsigned n)
{
	if (d->capacity - d->n < n)
		dynamic_add(&response, n);

	memcpy((char*)d->pt + d->n*d->item, buf, n*d->item);
	d->n += n;
}

void
dynamic_clear(struct dynamic *d)
{
	d->n = 0;
}

int
url_protocol(char *url)
{
	assert(url);
	if (str_starts_with(url, "http://")) return HTTP;
	if (str_starts_with(url, "https://")) return HTTPS;
	if (str_starts_with(url, "gemini://")) return GEMINI;
	if (str_starts_with(url, "gopher://")) return GOPHER;
	return 0;
}

char *
url_host(char *url)
{
	static char buf[4096];
	char *beg, *end;
	size_t n;

	assert(url);

	beg = (beg = strstr(url, "://")) ? beg+3 : url;	/* Skip protocol */
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
url_port(char *url)
{
	char *beg;
	assert(url);
	beg = (beg = strstr(url, "://")) ? beg+3 : url;	/* Skip protocol */
	return (beg = strchr(beg, ':')) ? atoi(beg+1) : 0;
}

char *
url_path(char *url)
{
	char *beg;
	assert(url);
	beg = (beg = strstr(url, "://")) ? beg+3 : url;	/* Skip protocol */
	return strchr(beg, '/');
}

why_t
net_fetch_secure(int sfd, char *host, char *msg)
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
	 * would be a mess with those many return statements. */
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
		dynamic_append(&response, buf, n);

	return 0;
}

why_t
net_fetch_plain(int sfd, char *msg)
{
	char buf[BUFSIZ];
	ssize_t n;

	if (send(sfd, msg, strlen(msg), 0) == -1)
		return "Failed to send request to server";

	if (send(sfd, CRLF, sizeof(CRLF), 0) == -1)
		return "Failed to send CRLF to server";

	while ((n = recv(sfd, buf, sizeof buf, 0)))
		dynamic_append(&response, buf, n);

	return 0;
}

why_t
net_fetch(char *host, int port, int ssl, char *msg)
{
	why_t why;
	int i, sfd;
	struct hostent *he;
	struct sockaddr_in addr;

	assert(host);
	assert(port > 0);
	assert(msg);

	if ((sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		return "Failed to open socket";

	if ((he = gethostbyname(host)) == 0)
		return "Failed to get hostname";

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	for (i=0; he->h_addr_list[i]; i++) {
		memcpy(&addr.sin_addr.s_addr, he->h_addr_list[i], sizeof(in_addr_t));
		if (!connect(sfd, (struct sockaddr*)&addr, sizeof addr))
			break;	/* Success */
	}

	if (!he->h_addr_list[i])
		return "Failed to connect, invalid port?";

	if (ssl) {
		why = net_fetch_secure(sfd, host, msg);
	} else {
		why = net_fetch_plain(sfd, msg);
	}

	if (close(sfd))
		return "Failed to close socket";

	if (why)
		return why;

	return 0;
}

void
parse_response(int protocol)
{
	char *body;
	unsigned n;

	body = response.pt;
	switch (protocol) {
	case HTTP:
	case HTTPS:
		body = strstr(response.pt, CRLF CRLF);
		if (body)
			body += strlen(CRLF)*2;
		break;
	case GEMINI:
		body = strstr(response.pt, CRLF);
		if (body)
			body += 2;
		break;
	}

	if (!body)
		body = response.pt;

	n = response.n - (body - (char*)response.pt);

	printf("%.*s", (int)n, body);
}

void
run(char *url)
{
	why_t why;
	char *host, *path, msg[4096];
	int protocol, port, ssl=0;

	protocol = url_protocol(url);
	port = url_port(url);
	host = url_host(url);
	path = url_path(url);

	if (!protocol)
		protocol = port;

	if (!port)
		port = protocol;

	switch (protocol) {
	case HTTP:
		snprintf(msg, sizeof msg, "GET http://%s%s HTTP/1.0\n",
			 host, path ? path : "/");
		break;
	case HTTPS:
		snprintf(msg, sizeof msg, "GET %s HTTP/1.0\nHost: %s\n",
			 path ? path : "/", host);
		ssl = 1;
		break;
	case GEMINI:
		snprintf(msg, sizeof msg, "gemini://%s%s\n",
			 host, path ? path : "/");
		ssl = 1;
		break;
	case GOPHER:
		snprintf(msg, sizeof msg, "%s\n", path ? path : "");
		break;
	default:
		errx(1, "Unknown protocol");
		break;
	}

	printf("protoc	%d\n", protocol);
	printf("host	%s\n", host);
	printf("port	%d\n", port);
	printf("ssl	%d\n", ssl);
	printf("msg	%s\n", msg);

	dynamic_clear(&response);
	why = net_fetch(host, port, ssl, msg);

	if (why)
		errx(1, "Error: %s", why);

	/* TODO(irek): At this point I should parse reponse header,
	 * response code, or response metadata depending on protocol.
	 * It's important because depending on response or in case of
	 * Gopher depending on request parsing is optional. */
	parse_response(protocol);
}

int
main(int argc, char **argv)
{
	char *url;

	if (argc < 2)
		errx(0, help, argv[0]);

	url = argv[1];
	str_tolower(url);

	dynamic_init(&response, sizeof(char), 4096);
	dynamic_init(&nodes, sizeof(struct node), 128);
	dynamic_init(&attrs, sizeof(struct attr), 128);

	run(url);

	return 0;
}
