/* Yupa v0.5 by irek@gabr.pl */

#include <assert.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#define BSIZ    BUFSIZ          /* Size of generic buffer */
#define HSIZ    64              /* Size of tab browsing history */
#define FMAX    FILENAME_MAX    /* Max size of buffer for file path */
#define LOG_LEVEL -1            /* Range from -1 (all) to 2 (only errors) */

#define LOG_IMPLEMENTATION
#include "log.h"
#include "arg.h"
#include "uri.h"
#include "gph.c"

_Static_assert(BSIZ > URI_SIZ, "BSIZ too small for URI_SIZ");
_Static_assert(BSIZ > FMAX,    "BSIZ too small for FMAX");

enum cmd {                      /* Commands possible to input in prompt */
	CMD_NUL = 0,            /* Empty command */
	CMD_URI,                /* Absolute URI string */
	CMD_LINK,               /* Index to link on current page */
	CMD_RELOAD,             /* Reload current page */
	CMD_TAB_NEW,            /* Create new tab */
	CMD_TAB_PREV,           /* Switch to previous tab */
	CMD_TAB_NEXT,           /* Switch to next tab */
	CMD_TAB_DUPLICATE,      /* Duplicate current tab */
	CMD_TAB_CLOSE,          /* Close current tab */
	CMD_HISTORY_PREV,       /* Goto previous browsing history page */
	CMD_HISTORY_NEXT,       /* Goto next browsing history page */
	CMD_RAW,                /* Show raw response */
	CMD_QUIT                /* Exit program */
};

enum filename {                 /* File names used by tab */
	FN_RAW = 0,             /* Raw response body */
	FN_FMT,                 /* Formatted raw content */
	_FN_SIZ                 /* For array size */
};

struct tab {
	int     index;                  /* Tab index */
	enum uri_protocol protocol;     /* Current page URI protocol */
	char    fn[_FN_SIZ][FMAX];      /* File paths */
	int     show;                   /* FN index of file to show, -1=none */
	char    history[HSIZ][URI_SIZ]; /* Browsing history */
	size_t  hi;                     /* Index to current history item */
	struct tab *prev, *next;        /* Double linked list */
};

static struct tab *s_tab = 0;           /* Pointer to current tab */
static char *s_pager = "less -XI";      /* Pager default command */
char *argv0;                            /* First program arg, for arg.h */

/* Print usage help message. */
static void
usage(void)
{
	fprintf(stderr,
		"usage: %s [-h] [uri..]\n"
		"\n"
		"	-h	Print this help message.\n"
		"	[uri..]	List of URIs to open on startup.\n"
		"env	PAGER	Pager cmd (less -XI).\n"
		, argv0);
}

/* Return pointer to static string with random alphanumeric characters
 * of LEN length. */
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

/* Create temporary file in /tmp dir with PREFIX file name prefix.
 * DST string should point to buffer of FILENAME_MAX size where path
 * to create file will be stored. */
static void
tmpf(char *prefix, char *dst)
{
	int fd;
	do {
		sprintf(dst, "%s%s-%s", "/tmp/", prefix, strrand(6));
	} while (!access(dst, F_OK));
	if ((fd = open(dst, O_RDWR | O_CREAT)) == -1) {
		ERR("open %s:", dst);
	}
	if (close(fd)) {
		ERR("close %s:", dst);
	}
}

/* Create new empty tab and set it as current tab. */
static void
tab_new(void)
{
	int index;
	struct tab *tab, *new;
	if (!(new = malloc(sizeof(*new)))) {
		ERR("malloc:");
	}
	memset(new, 0, sizeof(*new));
	tmpf("yupa.raw", new->fn[FN_RAW]);
	tmpf("yupa.fmt", new->fn[FN_FMT]);
	new->prev = s_tab;
	if (s_tab) {
		index = s_tab->index;
		new->index = ++index;
		new->next = s_tab->next;
		if (s_tab->next) {
			s_tab->next->prev = new;
		}
		s_tab->next = new;
		for (tab = new->next; tab; tab = tab->next) {
			tab->index = ++index;
		}
	}
	s_tab = new;
}

/**/
static void
tab_close(void)
{
	struct tab *tab = s_tab;
	if (unlink(tab->fn[FN_RAW]) == -1) {
		WARN("unlink:");
	}
	if (unlink(tab->fn[FN_FMT]) == -1) {
		WARN("unlink:");
	}
	if (tab->next) {
		tab->next->prev = tab->prev;
		if (tab->prev) {
			tab->prev->next = tab->next;
		}
		s_tab = tab->next;
	} else if (tab->prev) {
		tab->prev->next = 0;
		s_tab = tab->prev;
	} else {
		s_tab = 0;
	}
}

/* Add new URI to current tab history. */
static void
history_add(char *uri)
{
	if (s_tab->history[s_tab->hi % HSIZ][0]) {
		s_tab->hi++;
	}
	strncpy(s_tab->history[s_tab->hi % HSIZ], uri, URI_SIZ);
	/* Make next history item empty to cut off old forward history
	 * every time the new item is being added. */
	s_tab->history[(s_tab->hi + 1) % HSIZ][0] = 0;
}

/* Get current tab history item shifting history index by SHIFT. */
static char *
history_get(int shift)
{
	if (shift > 0 && !s_tab->history[(s_tab->hi + 1) % HSIZ][0]) {
		return 0;
	}
	if (shift < 0 && !s_tab->hi) {
		return 0;
	}
	s_tab->hi += shift;
	return s_tab->history[s_tab->hi % HSIZ];
}

/* Use pager to print content of FILENAME. */
static void
show(char *filename)
{
	char buf[BSIZ];
	sprintf(buf, "%s %s", s_pager, filename);
	system(buf);
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

/**/
static int
onuri(char *uri)
{
	enum uri_protocol protocol;
	int sfd, port;
	char buf[BSIZ], *host, *path, item = GPH_ITEM_GPH;
	FILE *raw, *fmt;
	ssize_t ssiz;
	DEV("%s", uri);
	if (!uri || !uri[0]) {
		return 0;
	}
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
		WARN("Only gopher protocol is supported");
		return 0;
	}
	if (path && path[1]) {
		item = path[1];
		path += 2;
	}
	if ((sfd = req(host, port, path)) == 0) {
		printf("Invalid URI %s\n", uri);
		return 0;
	}
	if (!(raw = fopen(s_tab->fn[FN_RAW], "w+"))) {
		ERR("fopen %s %s:", uri, s_tab->fn[FN_RAW]);
	}
	while ((ssiz = recv(sfd, buf, sizeof(buf), 0)) > 0) {
		if (fwrite(buf, 1, ssiz, raw) != (size_t)ssiz) {
			ERR("fwrite %s:", uri);
		}
	}
	if (ssiz < 0) {
		ERR("recv %s:", uri);
	}
	if (close(sfd)) {
		ERR("close %s %d:", uri, sfd);
	}
	s_tab->protocol = protocol;
	switch (item) {
	case GPH_ITEM_TXT:
		s_tab->show = FN_RAW;
		break;
	case GPH_ITEM_GPH:
		s_tab->show = FN_FMT;
		break;
	default:
		s_tab->show = -1;
	}
	DEV("item %c", item);
	DEV("show %d", s_tab->show);
	if (s_tab->show == -1) {
		/* TODO(irek): Flow of closing this file is ugly.
		 * This probably could be refactored with some good
		 * old goto. */
		if (fclose(raw) == EOF) {
			ERR("fclose %s %s:", uri, s_tab->fn[FN_RAW]);
		}
		printf("Not a Gopher submenu and not a text file\n");
		return 0;
	}
	if (s_tab->show == FN_FMT) {
		if (!(fmt = fopen(s_tab->fn[FN_FMT], "w"))) {
			ERR("fopen %s %s:", uri, s_tab->fn[FN_FMT]);
		}
		gph_format(raw, fmt);
		if (fclose(fmt) == EOF) {
			ERR("fclose %s %s:", uri, s_tab->fn[FN_FMT]);
		}
	}
	if (fclose(raw) == EOF) {
		ERR("fclose %s %s:", uri, s_tab->fn[FN_RAW]);
	}
	show(s_tab->fn[s_tab->show]);
	return 1;
}

/**/
static char *
link_get(int index)
{
	char *uri;
	FILE *raw;
	assert(index > 0);
	if (s_tab->protocol != URI_PROTOCOL_GOPHER) {
		WARN("Only gopher protocol is supported");
		return 0;
	}
	if (!(raw = fopen(s_tab->fn[FN_RAW], "r"))) {
		ERR("fopen %s:", s_tab->fn[FN_RAW]);
	}
	uri = gph_uri(raw, index);
	if (fclose(raw) == EOF) {
		ERR("fclose %s:", s_tab->fn[FN_RAW]);
	}
	return uri;
}

static void
onquit(void)
{
	while (s_tab) {
		tab_close();
	}
	exit(0);
}

/* Return non 0 value when STR contains only digits. */
static int
isnum(char *str)
{
	if (*str == 0) {
		return 0;
	}
	for (; *str; str++) {
		if (*str < '0' || *str > '9') {
			return 0;
		}
	}
	return 1;
}

/* TODO(irek): I would like to have it as data instead of as logic.
 * Also at the moment commands don't take arguments.  Something to
 * think about later. */
/**/
static enum cmd
cmd(char *buf, size_t siz)
{
	if (siz == 0)           return CMD_NUL;
#define _CMD_IS(_str) strlen(_str) == siz && !strncmp(_str, buf, siz)
	if (_CMD_IS("q"))       return CMD_QUIT;
	if (_CMD_IS("quit"))    return CMD_QUIT;
	if (_CMD_IS("exit"))    return CMD_QUIT;
	if (_CMD_IS("0"))       return CMD_RELOAD;
	if (_CMD_IS("r"))       return CMD_RELOAD;
	if (_CMD_IS("reload"))  return CMD_RELOAD;
	if (_CMD_IS("refresh")) return CMD_RELOAD;
	if (_CMD_IS("R"))       return CMD_RAW;
	if (_CMD_IS("raw"))     return CMD_RAW;
	if (_CMD_IS("T"))       return CMD_TAB_NEW;
	if (_CMD_IS("tab"))     return CMD_TAB_NEW;
	if (_CMD_IS("P"))       return CMD_TAB_PREV;
	if (_CMD_IS("tp"))      return CMD_TAB_PREV;
	if (_CMD_IS("tprev"))   return CMD_TAB_PREV;
	if (_CMD_IS("N"))       return CMD_TAB_NEXT;
	if (_CMD_IS("tn"))      return CMD_TAB_NEXT;
	if (_CMD_IS("tnext"))   return CMD_TAB_NEXT;
	if (_CMD_IS("D"))       return CMD_TAB_DUPLICATE;
	if (_CMD_IS("tdup"))    return CMD_TAB_DUPLICATE;
	if (_CMD_IS("X"))       return CMD_TAB_CLOSE;
	if (_CMD_IS("C"))       return CMD_TAB_CLOSE;
	if (_CMD_IS("tclose"))  return CMD_TAB_CLOSE;
	if (_CMD_IS("b"))       return CMD_HISTORY_PREV;
	if (_CMD_IS("back"))    return CMD_HISTORY_PREV;
	if (_CMD_IS("p"))       return CMD_HISTORY_PREV;
	if (_CMD_IS("prev"))    return CMD_HISTORY_PREV;
	if (_CMD_IS("n"))       return CMD_HISTORY_NEXT;
	if (_CMD_IS("next"))    return CMD_HISTORY_NEXT;
	if (isnum(buf))         return CMD_LINK;
	return CMD_URI;
}

/**/
static void
run(void)
{
	char buf[BSIZ], *uri;
	size_t len;
	while (1) {
		fprintf(stderr, "yupa(%d%s)> ",
			s_tab->index,
			s_tab->next ? "+" : "");
		if (!fgets(buf, sizeof(buf), stdin)) {
			continue;
		}
		len = strlen(buf) - 1;
		buf[len] = 0;
		switch (cmd(buf, len)) {
		case CMD_URI:
			if (onuri(buf)) {
				history_add(buf);
			}
			break;
		case CMD_LINK:
			uri = link_get(atoi(buf));
			if (onuri(uri)) {
				history_add(uri);
			}
			break;
		case CMD_RELOAD:
			onuri(history_get(0));
			break;
		case CMD_RAW:
			show(s_tab->fn[FN_RAW]);
			break;
		case CMD_TAB_NEW:
			tab_new();
			break;
		case CMD_TAB_PREV:
			if (!s_tab->prev) {
				break;
			}
			s_tab = s_tab->prev;
			show(s_tab->fn[s_tab->show]);
			break;
		case CMD_TAB_NEXT:
			if (!s_tab->next) {
				break;
			}
			s_tab = s_tab->next;
			show(s_tab->fn[s_tab->show]);
			break;
		case CMD_TAB_DUPLICATE:
			uri = history_get(0);
			tab_new();
			if (onuri(uri)) {
				history_add(uri);
			}
			break;
		case CMD_TAB_CLOSE:
			if (!s_tab->prev && !s_tab->next) {
				printf("Can't close last tab\n");
				break;
			}
			tab_close();
			break;
		case CMD_HISTORY_PREV:
			onuri(history_get(-1));
			break;
		case CMD_HISTORY_NEXT:
			onuri(history_get(+1));
			break;
		case CMD_QUIT:
			onquit();
			break;
		case CMD_NUL:
		default:
			DEV("TODO Print list of commands.");
			break;
		}
	}
}

int
main(int argc, char *argv[])
{
	char *env;
	int i;
	if ((env = getenv("PAGER"))) {
		s_pager = env;
	}
	ARGBEGIN {
	case 'h':
		usage();
		return 0;
	default:
		usage();
		return 1;
	} ARGEND
	for (i = 0; i < argc; i++) {
		tab_new();
		if (onuri(argv[i])) {
			history_add(argv[0]);
		}
	}
	if (!s_tab) {
		tab_new();
	}
	run();
	return 0;
}
