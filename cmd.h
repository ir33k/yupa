// Prompt commands.

enum action {                   // Possible action to input in cmd prompt
	A_NUL = 0,              // Empty action
	A_URI,                  // Absolute URI string (default action)
	A_LINK,                 // Index to link on current page
	// Cmd actions
	A_QUIT,                 // Exit program
	A_HELP,                 // Print program help
	A_REPEAT,               // Repeat last command
	A_CANCEL,               // Cancel insertion of current command
	A_PAGE_GET,             // Get (reload) current page
	A_PAGE_BODY,            // Show response body
	A_HIS_LIST,             // Print history list of current page
	A_HIS_PREV,             // Goto previous browsing history page
	A_HIS_NEXT,             // Goto next browsing history page
	A_TAB_ADD,              // Add new tab
	A_TAB_PREV,             // Switch to previous tab
	A_TAB_NEXT,             // Switch to next tab
	A_TAB_DUP,              // Duplicate current tab
	A_TAB_CLOSE,            // Close current tab
};

struct cmd {
	enum action action;
	// First char in NAME is command key and when fourth char is
	// '+' then ACTION is used as index to CMD array.
	const char *name;
};

static struct cmd cmd_tree[] = {
[0] =	{ A_QUIT,      "q: quit" },
	{ A_HELP,      "h: help" },
	{ 10,          "p: +page" },
	{ 30,          "t: +tabs" },
	{ A_REPEAT,    ".: cmd-repeat-last" },
	{ A_CANCEL,    "-: cmd-cancel" },
	{0},
[10] =	{ A_PAGE_GET,  "p: page-reload" },
	{ A_PAGE_BODY, "b: page-show-res-body" },
	{ 20,          "h: +page-history" },
	{ 0,           "-: +cmd-cancel" },
	{0},
[20] =	{ A_HIS_LIST,  "h: history-list" },
	{ A_HIS_PREV,  "p: history-goto-prev" },
	{ A_HIS_NEXT,  "n: history-goto-next" },
	{ 10,          "-: +cmd-cancel" },
	{0},
[30] =	{ A_TAB_ADD,   "t: tab-add" },
	{ A_TAB_PREV,  "p: tab-goto-prev" },
	{ A_TAB_NEXT,  "n: tab-goto-next" },
	{ A_TAB_DUP,   "d: tab-duplicat" },
	{ A_TAB_CLOSE, "c: tab-close" },
	{ 0,           "-: +cmd-cancel" },
	{0},
};

//
static enum action
cmd_action(struct cmd *cmd, char buf[BSIZ])
{
	size_t i, c=0, b=0;
	assert(cmd);
	assert(buf);
	while (1) {
		for (; buf[b] >= ' '; b++) {
			while (cmd[c].name && cmd[c].name[0] != buf[b]) c++;
			if (!cmd[c].name) {
				return A_URI;
			}
			if (cmd[c].name[3] == '+') {
				c = cmd[c].action;
				continue;
			}
			if (buf[b+1] >= ' ') {
				return A_URI;
			}
			return cmd[c].action;   // Found action
		}
		for (i = c; cmd[i].name; i++) {
			printf("\t%s\n", cmd[i].name);
		}
		if (BSIZ - b <= 0) {
			return A_URI;
		}
		printf("cmd> %.*s", (int)b, buf);
		fgets(buf+b, BSIZ-b, stdin);
	}
	ERR("Unreachable");
}
