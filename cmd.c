#include <stdio.h>
#include <assert.h>
#include "cmd.h"
#include "uri.h"
#include "logerr.h"

const struct cmd cmd_tree[] = {
[0] =	{ CMD_A_QUIT,           "q: quit" },
	{ CMD_A_HELP,           "h: help" },
	{ 20,                   "p: +page" },
	{ 60,                   "t: +tabs" },
	{ CMD_A_REPEAT,         ".: cmd-repeat-last" },
	{ CMD_A_CANCEL,         "-: cmd-cancel" },
	{0},
[20] =	{ CMD_A_PAGE_GET,       "p: page-reload" },
	{ CMD_A_PAGE_BODY,      "b: page-show-res-body" },
	{ 40,                   "h: +page-history" },
	{ 0,                    "-: +cmd-cancel" },
	{0},
[40] =	{ CMD_A_HIS_LIST,       "h: history-list" },
	{ CMD_A_HIS_PREV,       "p: history-goto-prev" },
	{ CMD_A_HIS_NEXT,       "n: history-goto-next" },
	{ 10,                   "-: +cmd-cancel" },
	{0},
[60] =	{ CMD_A_TAB_LIST,       "t[index]: tab-list" },
	{ CMD_A_TAB_ADD,        "a: tab-add" },
	{ CMD_A_TAB_PREV,       "p: tab-goto-prev" },
	{ CMD_A_TAB_NEXT,       "n: tab-goto-next" },
	{ CMD_A_TAB_DUP,        "d: tab-duplicat" },
	{ CMD_A_TAB_CLOSE,      "c: tab-close" },
	{ 0,                    "-: +cmd-cancel" },
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

//
enum cmd_action
cmd_action(const struct cmd *cmd, char buf[BUFSIZ], char **arg)
{
	size_t i, c=0, b=0;
	assert(cmd);
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
			while (cmd[c].name && cmd[c].name[0] != buf[b]) c++;
			if (!cmd[c].name) {
				return 0;
			}
			if (cmd[c].name[3] == '+') {
				c = cmd[c].action;
				continue;
			}
			*arg = buf + b + 1;
			return cmd[c].action;   // Found action
		}
		for (i = c; cmd[i].name; i++) {
			printf("\t%s\n", cmd[i].name);
		}
		if (BUFSIZ - b <= 0) {
			return 0;
		}
		printf("cmd> %.*s", (int)b, buf);
		fgets(buf+b, BUFSIZ-b, stdin);
	}
	ERR("Unreachable");
}
