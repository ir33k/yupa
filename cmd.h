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
	int next;
	enum action action;
	const char c, *name;
};

static struct cmd cmd_tree[] = {
[0] =	{ 0, A_QUIT,      'q', "quit" },
	{ 0, A_HELP,      'h', "help" },
	{ 1, 10,          'p', "page" },
	{ 1, 30,          't', "tabs" },
	{ 0, A_REPEAT,    '.', "cmd-repeat-last" },
	{ 0, A_CANCEL,    '-', "cmd-cancel" },
	{0},
[10] =	{ 0, A_PAGE_GET,  'p', "page-reload" },
	{ 0, A_PAGE_BODY, 'b', "page-show-res-body" },
	{ 1, 20,          'h', "page-history" },
	{ 1, 0,           '-', "cmd-cancel" },
	{0},
[20] =	{ 0, A_HIS_LIST,  'h', "history-list" },
	{ 0, A_HIS_PREV,  'b', "history-goto-prev" },
	{ 0, A_HIS_NEXT,  'f', "history-goto-next" },
	{ 1, 10,          '-', "cmd-cancel" },
	{0},
[30] =	{ 0, A_TAB_ADD,   't', "tab-add" },
	{ 0, A_TAB_PREV,  'p', "tab-goto-prev" },
	{ 0, A_TAB_NEXT,  'n', "tab-goto-next" },
	{ 0, A_TAB_DUP,   'd', "tab-duplicat" },
	{ 0, A_TAB_CLOSE, 'c', "tab-close" },
	{ 1, 0,           '-', "cmd-cancel" },
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
			while (cmd[c].c != buf[b]) c++;
			if (!cmd[c].c) {
				return A_URI;
			}
			if (cmd[c].next) {
				c = cmd[c].action;
				continue;
			}
			if (buf[b+1] >= ' ') {
				return A_URI;
			}
			return cmd[c].action;
		}
		for (i = c; cmd[i].name; i++) {
			printf("\t%c: %s%s\n",
			       cmd[i].c,
			       cmd[i].next ? "+" : "",
			       cmd[i].name);
		}
		if (BSIZ - b <= 0) {
			return A_URI;
		}
		printf("cmd> %.*s", (int)b, buf);
		fgets(buf+b, BSIZ-b, stdin);
	}
	ERR("Unreachable");
}
