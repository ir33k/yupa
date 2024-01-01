#include <assert.h>
#include <stdio.h>
#include "cmd.h"
#include "uri.h"

enum {                          // Indexes of first item in cmd group
	ROOT =  0,
	PAGE = 20,
	HIST = 40,
	TAB  = 60,
	SH   = 80,
};

struct cmd {
	int next;
	// First char in NAME is command key and when second char
	// is '+' then ACTION is used as index to CMD array.
	const char *name;
};

static const struct cmd tree[] = {
[ROOT]=	{ CMD_A_QUIT,           "q: quit" },
	{ CMD_A_HELP,           "h: help" },
	{ PAGE,                 "p+ page" },
	{ TAB,                  "t+ tabs" },
	{ SH,                   "!+ shell" },
	{ CMD_A_REPEAT,         ".: cmd-repeat-last" },
	{ CMD_A_CANCEL,         ",: cmd-cancel" },
	{0},
[PAGE]=	{ CMD_A_PAGE_GET,       "p: page-reload" },
	{ CMD_A_PAGE_RAW,       "b: page-show-raw-response" },
	{ HIST,                 "h+ page-history" },
	{ ROOT,                 ",+ cmd-cancel" },
	{0},
[HIST]=	{ CMD_A_HIS_LIST,       "h: history-list" },
	{ CMD_A_HIS_PREV,       "p: history-goto-prev" },
	{ CMD_A_HIS_NEXT,       "n: history-goto-next" },
	{ PAGE,                 ",+ cmd-cancel" },
	{0},
[TAB]=	{ CMD_A_TAB_GOTO,       "t[tab_index]: tab-goto-list" },
	{ CMD_A_TAB_OPEN,       "o[uri/link]: tab-open" },
	{ CMD_A_TAB_ADD,        "a: tab-add" },
	{ CMD_A_TAB_PREV,       "p: tab-goto-prev" },
	{ CMD_A_TAB_NEXT,       "n: tab-goto-next" },
	{ CMD_A_TAB_CLOSE,      "c: tab-close" },
	{ ROOT,                 ",+ cmd-cancel" },
	{0},
[SH]=	{ CMD_A_SH_RAW,         "!<cmd>: shell-cmd-on-raw-response" },
	{ CMD_A_SH_FMT,         "f<cmd>: shell-cmd-on-fmt-response" },
	{ ROOT,                 ",+ cmd-cancel" },
	{0},
};

// Return non 0 value when STR contains only digits.
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

enum cmd_action
cmd_action(char buf[BUFSIZ], char **arg)
{
	size_t i, c=0, b=0;
	assert(buf);
	*arg = 0;
	if (uri_protocol(buf)) {
		return CMD_A_URI;
	}
	if (isnum(buf)) {
		return CMD_A_LINK;
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
		printf("cmd> %.*s", (int)b, buf);
		fgets(buf+b, BUFSIZ-b, stdin);
	}
	assert(0 && "unreachable");
}
