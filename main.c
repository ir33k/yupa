#define NAME    "Yupa"
#define VERSION "v0.9"
#define AUTHOR  "irek@gabr.pl"

#include <assert.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include "arg.h"
#include "uri.h"
#include "cmd.h"

#define LOGERR_IMPLEMENTATION
#include "logerr.h"

#define BSIZ    BUFSIZ          // Size of generic buffer
#define HSIZ    64              // Size of tab browsing history
#define FMAX    FILENAME_MAX    // Max size of buffer for file path

_Static_assert(BSIZ > URI_SIZ, "BSIZ too small for URI_SIZ");
_Static_assert(BSIZ > FMAX,    "BSIZ too small for FMAX");

#include "gph.c"

enum filename {                 // File names used by tab
	FN_BODY = 0,            // Response body
	FN_FMT,                 // Formatted BODY content
	_FN_SIZ                 // For array size
};

struct tab {                            // Tab node in double liked list
	struct tab *prev, *next;        // Previous and next nodes
	int     protocol;               // Current page URI protocol
	char    fn[_FN_SIZ][FMAX];      // File paths
	int     show;                   // FN index of file to show, -1=none
	char    history[HSIZ][URI_SIZ]; // Browsing history
	size_t  hi;                     // Index to current history item
};

static struct tab *s_tab  = 0;          // Pointer to current tab
static int         s_tabc = 1;          // Tabs count
static int         s_tabi = 1;          // 1 based index of current tab
static char       *s_pager;             // Pager default command

static const char *s_help =
	NAME " " VERSION " by " AUTHOR "\n"
	"\n"
	"Gopher protocol CLI browser with tabs and browsing history.\n"
	"Browse by inserting absolute URI or link index from current page.\n"
	"Press RETURN to open commands menu or insert command upfront.\n"
	"Prompt indicate (current_tab_number/number_of_all_tabs).\n"
	"Run program with -h flag to read about arguments and env vars.\n"
	"\n";

char *argv0;                    // First program arg, for arg.h

// Print usage help message.
static void
usage(void)
{
	printf("usage: %s [-v] [-h] [uri..]\n"
	       "\n"
	       "	-v	Print version.\n"
	       "	-h	Print this help message.\n"
	       "	[uri..]	List of URIs to open on startup.\n"
	       "env	PAGER	Pager cmd (less -XI).\n"
	       , argv0);
}

// Return pointer to static string with random alphanumeric characters
// of LEN length.
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

// Create temporary file in /tmp dir with PREFIX file name prefix.
// DST string should point to buffer of FILENAME_MAX size where path
// to create file will be stored.
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

// Add new URI to current tab history.
static void
history_add(char *uri)
{
	if (s_tab->history[s_tab->hi % HSIZ][0]) {
		s_tab->hi++;
	}
	strncpy(s_tab->history[s_tab->hi % HSIZ], uri, URI_SIZ);
	// Make next history item empty to cut off old forward history
	// every time the new item is being added.
	s_tab->history[(s_tab->hi + 1) % HSIZ][0] = 0;
}

// Get current tab history item shifting history index by SHIFT.
static char *
history_get(int shift)
{
	if (shift == 0) {
		return s_tab->history[s_tab->hi % HSIZ];
	}
	// TODO(irek): I expect that there is a bug when you loop over
	// the HSIZ and then try to get back.  I'm not checking for
	// the empty history entry.  Also when going forward I'm
	// looking only at very next entry but value of SHIFT can be
	// more then 1.
	if (shift > 0 && !s_tab->history[(s_tab->hi + 1) % HSIZ][0]) {
		return 0;
	}
	if (shift < 0 && !s_tab->hi) {
		return 0;
	}
	s_tab->hi += shift;
	return s_tab->history[s_tab->hi % HSIZ];
}

// Add new empty tab and set it as current tab.
static void
tab_add(void)
{
	struct tab *tab;
	if (!(tab = malloc(sizeof(*tab)))) {
		ERR("malloc:");
	}
	memset(tab, 0, sizeof(*tab));
	tmpf("yupa.body", tab->fn[FN_BODY]);
	tmpf("yupa.fmt",  tab->fn[FN_FMT]);
	tab->prev = s_tab;
	if (s_tab) {
		tab->next = s_tab->next;
		if (s_tab->next) {
			s_tab->next->prev = tab;
		}
		s_tab->next = tab;
		s_tabc++;
		s_tabi++;
	}
	s_tab = tab;
}

static void
tab_prev(void)
{
	if (!s_tab->prev) {
		return;
	}
	s_tab = s_tab->prev;
	s_tabi--;
	printf("Tab %d: %s\n", s_tabi, history_get(0));
}

static void
tab_next(void)
{
	if (!s_tab->next) {
		return;
	}
	s_tab = s_tab->next;
	s_tabi++;
	printf("Tab %d: %s\n", s_tabi, history_get(0));
}

static void
tab_goto(int index)
{
	if (index < 1 || index > s_tabc) {
		return;
	}
	while (s_tab->prev) s_tab = s_tab->prev;
	s_tabi = index;
	while (--index) s_tab = s_tab->next;
	printf("Tab %d: %s\n", s_tabi, history_get(0));
}

//
static void
tab_close(void)
{
	struct tab *tab = s_tab;
	if (unlink(tab->fn[FN_BODY]) == -1) {
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
		s_tabi--;
	} else {
		s_tab = 0;
	}
	s_tabc--;
}

static void
ontab_list(void)
{
	struct tab *tab = s_tab;
	int i;
	while (tab->prev) tab = tab->prev;
	for (i = 1; tab; tab = tab->next, i++) {
		printf("\t%d: %s%s\n", i,
		       i == s_tabi ? "> " : "  ",
		       tab->history[tab->hi % HSIZ]);
	}
}

// Use pager to print content of FILENAME.
static void
show(char *filename)
{
	char buf[BSIZ];
	sprintf(buf, "%s %s", s_pager, filename);
	system(buf);
}

// Establish AF_INET internet SOCK_STREAM stream connection to HOST of
// PORT.  Return socket file descriptor or 0 on error.
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
		return sfd;	// Success
	}
	WARN("failed to connect with %s %d:", host, port);
	return 0;
}

//
// TODO(irek): I dislike this function.  Merging it with onuri?

// Open connection to server under HOST with PORT and optional PATH.
// Return socket file descriptor on success that is ready to read
// server response.  Return 0 on error.
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

//
static int
onuri(char *uri)
{
	int sfd, protocol, port;
	char buf[BSIZ], *host, *path, item = GPH_ITEM_GPH;
	FILE *body, *fmt;
	ssize_t ssiz;
	LOG("%s", uri);
	if (!uri || !uri[0]) {
		return 0;
	}
	assert(strlen(uri) <= URI_SIZ);
	protocol = uri_protocol(uri);
	host = uri_host(uri);
	port = uri_port(uri);
	path = uri_path(uri);
	if (!port) {
		port = protocol ? protocol : URI_GOPHER;
	}
	if (!protocol) {
		protocol = port;
	}
	if (protocol != URI_GOPHER) {
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
	if (!(body = fopen(s_tab->fn[FN_BODY], "w+"))) {
		ERR("fopen %s %s:", uri, s_tab->fn[FN_BODY]);
	}
	while ((ssiz = recv(sfd, buf, sizeof(buf), 0)) > 0) {
		if (fwrite(buf, 1, ssiz, body) != (size_t)ssiz) {
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
		s_tab->show = FN_BODY;
		break;
	case GPH_ITEM_GPH:
		s_tab->show = FN_FMT;
		break;
	default:
		s_tab->show = -1;
	}
	LOG("item %c", item);
	LOG("show %d", s_tab->show);
	if (s_tab->show == -1) {
		// TODO(irek): Flow of closing this file is ugly.
		// This probably could be refactored with some good
		// old goto.
		if (fclose(body) == EOF) {
			ERR("fclose %s %s:", uri, s_tab->fn[FN_BODY]);
		}
		printf("Not a Gopher submenu and not a text file\n");
		return 0;
	}
	if (s_tab->show == FN_FMT) {
		if (!(fmt = fopen(s_tab->fn[FN_FMT], "w"))) {
			ERR("fopen %s %s:", uri, s_tab->fn[FN_FMT]);
		}
		gph_format(body, fmt);
		if (fclose(fmt) == EOF) {
			ERR("fclose %s %s:", uri, s_tab->fn[FN_FMT]);
		}
	}
	if (fclose(body) == EOF) {
		ERR("fclose %s %s:", uri, s_tab->fn[FN_BODY]);
	}
	show(s_tab->fn[s_tab->show]);
	return 1;
}

//
static char *
link_get(int index)
{
	char *uri = 0;
	FILE *body;
	assert(index > 0);
	if (s_tab->protocol != URI_GOPHER) {
		WARN("Only gopher protocol is supported");
		return 0;
	}
	if (!(body = fopen(s_tab->fn[FN_BODY], "r"))) {
		ERR("fopen %s:", s_tab->fn[FN_BODY]);
	}
	switch (s_tab->protocol) {
	case URI_GOPHER:
		uri = gph_uri(body, index);
		break;
	case URI_GEMINI:
	case URI_FILE:
	case URI_FTP:
	case URI_SSH:
	case URI_FINGER:
	case URI_HTTP:
	case URI_HTTPS:
	case URI_NUL:
	default:
		WARN("Unsupported protocol %d %s", s_tab->protocol,
		     uri_protocol_str(s_tab->protocol));
	}
	if (fclose(body) == EOF) {
		ERR("fclose %s:", s_tab->fn[FN_BODY]);
	}
	return uri;
}

//
static void
onprompt(char buf[BSIZ])
{
	static char last[BSIZ] = {0};
	char *arg, *uri;
	int i;
	switch (cmd_action(buf, &arg)) {
	case CMD_A_QUIT:
		while (s_tab) {
			tab_close();
		}
		exit(0);
		break;
	case CMD_A_HELP:
		printf(s_help);
		break;
	case CMD_A_REPEAT:
		if (last[0]) {
			strcpy(buf, last);
			onprompt(buf);
		}
		return; // Return to avoid defining CMD_A_REPEAT as last cmd
	case CMD_A_URI:
		if (onuri(buf)) {
			history_add(buf);
		}
		break;
	case CMD_A_LINK:
		uri = link_get(atoi(buf));
		if (onuri(uri)) {
			history_add(uri);
		}
		break;
	case CMD_A_PAGE_GET:
		onuri(history_get(0));
		break;
	case CMD_A_PAGE_BODY:
		show(s_tab->fn[FN_BODY]);
		break;
	case CMD_A_TAB_LIST:
		if ((i = atoi(arg))) {
			tab_goto(i);
		} else {
			ontab_list();
		}
		break;
	case CMD_A_TAB_ADD:
		tab_add();
		break;
	case CMD_A_TAB_PREV:
		tab_prev();
		break;
	case CMD_A_TAB_NEXT:
		tab_next();
		break;
	case CMD_A_TAB_OPEN:
		if (arg[0]) {
			uri = (i = atoi(arg)) ? link_get(i) : arg;
		} else {
			uri = history_get(0);
		}
		tab_add();
		if (onuri(uri)) {
			history_add(uri);
		}
		break;
	case CMD_A_TAB_CLOSE:
		if (!s_tab->prev && !s_tab->next) {
			printf("Can't close last tab\n");
			break;
		}
		tab_close();
		break;
	case CMD_A_HIS_LIST:
		WARN("Not implemented");
		break;
	case CMD_A_HIS_PREV:
		onuri(history_get(-1));
		break;
	case CMD_A_HIS_NEXT:
		onuri(history_get(+1));
		break;
	case CMD_A_CANCEL:
	case CMD_A_NUL:
		break;
	default:
		ERR("Unreachable");
		break;
	}
	strcpy(last, buf);
}

int
main(int argc, char *argv[])
{
	char *env, buf[BSIZ];
	int i;
	ARGBEGIN {
	case 'v':
		printf(VERSION "\n");
		return 0;
	case 'h':
		usage();
		return 0;
	default:
		usage();
		return 1;
	} ARGEND
	s_pager = (env = getenv("PAGER")) ? env : "less -XI";
	for (i = 0; i < argc; i++) {
		tab_add();
		if (onuri(argv[i])) {
			history_add(argv[i]);
		}
	}
	if (!s_tab) {
		tab_add();
	}
	while (1) {
		printf("yupa(%d/%d)> ", s_tabi, s_tabc);
		if (!fgets(buf, sizeof(buf), stdin)) {
			continue;
		}
		buf[strlen(buf)-1] = 0;
		onprompt(buf);
	}
	return 0;
}
