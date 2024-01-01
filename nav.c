#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "nav.h"
#include "uri.h"

enum {                          // Indexes to navigation groups
	ROOT =  0,              // Root nav menu
	PAGE = 10,              // Pages stuff
	HIST = 20,              // Page history submenu
	GET  = 30,              // Page download submenu
	TAB  = 40,              // Tabs manipulation
	SH   = 50,              // Shell commands execution
};

struct nav {
	int next;
	// First char in NAME is command key and when second char
	// is '+' then ACTION is used as index to submenu in array.
	const char *name;
};

static const struct nav tree[] = {
[ROOT]=	{ NAV_A_QUIT,           "q: quit" },
	{ NAV_A_HELP,           "h: help" },
	{ PAGE,                 "p+ page" },
	{ TAB,                  "t+ tabs" },
	{ SH,                   "!+ shell" },
	{ NAV_A_REPEAT,         ".: nav-repeat-last" },
	{ NAV_A_CANCEL,         ",: nav-cancel" },
	{0},
[PAGE]=	{ NAV_A_PAGE_GET,       "p: page-reload" },
	{ NAV_A_PAGE_RAW,       "b: page-show-raw-response" },
	{ HIST,                 "h+ page-history" },
	{ GET,                  "d+ page-download" },
	{ ROOT,                 ",+ nav-cancel" },
	{0},
[HIST]=	{ NAV_A_HIS_LIST,       "h: history-list" },
	{ NAV_A_HIS_PREV,       "p: history-goto-prev" },
	{ NAV_A_HIS_NEXT,       "n: history-goto-next" },
	{ PAGE,                 ",+ nav-cancel" },
	{0},
[GET]=	{ NAV_A_GET_RAW,        "d<path>: download-raw" },
	{ NAV_A_GET_FMT,        "f<path>: download-fmt" },
	{ PAGE,                 ",+ nav-cancel" },
	{0},
[TAB]=	{ NAV_A_TAB_GOTO,       "t[index]: tab-goto-list" },
	{ NAV_A_TAB_OPEN,       "o[uri/link]: tab-open" },
	{ NAV_A_TAB_ADD,        "a: tab-add" },
	{ NAV_A_TAB_PREV,       "p: tab-goto-prev" },
	{ NAV_A_TAB_NEXT,       "n: tab-goto-next" },
	{ NAV_A_TAB_CLOSE,      "c: tab-close" },
	{ ROOT,                 ",+ nav-cancel" },
	{0},
[SH]=	{ NAV_A_SH_RAW,         "!<cmd>: shell-cmd-on-raw-response" },
	{ NAV_A_SH_FMT,         "f<cmd>: shell-cmd-on-fmt-response" },
	{ ROOT,                 ",+ nav-cancel" },
	{0},
};

enum nav_action
nav_action(char buf[BUFSIZ], char **arg)
{
	size_t i, c=0, b=0;
	assert(buf);
	*arg = 0;
	if (atoi(buf)) {
		return NAV_A_LINK;
	}
	if (uri_protocol(buf)) {
		return NAV_A_URI;
	}
	while (1) {
		for (; buf[b] > ' '; b++) {
			while (tree[c].name && tree[c].name[0] != buf[b]) c++;
			if (!tree[c].name) {
				return 0;
			}
			if (tree[c].name[1] == '+') {
				c = tree[c].next;
				continue;
			}
			*arg = buf + b + 1;
			while (**arg && **arg <= ' ') (*arg)++;
			return tree[c].next;    // Found action
		}
		for (i = c; tree[i].name; i++) {
			printf("\t%s\n", tree[i].name);
		}
		if (BUFSIZ - b <= 0) {
			return 0;
		}
		printf("nav> %.*s", (int)b, buf);
		fgets(buf+b, BUFSIZ-b, stdin);
	}
	assert(0 && "unreachable");
}
