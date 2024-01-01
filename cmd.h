// Prompt commands.

enum action {                   // Possible action to input in cmd prompt
	A_NUL           = 0,    // Empty action
	A_URI,                  // Absolute URI string (default action)
	A_LINK,                 // Index to link on current page
	// Cmd actions
	A_QUIT,                 // Exit program
	A_HELP,                 // Print program help
	A_REPEAT,               // Repeat last command
	A_CANCEL,               // Cancel insertion of current command
	A_PAGE_RELOAD   = 100,  // Reload current page
	A_PAGE_BODY,            // Response body
	A_HIS_LIST,             // Print history list of current page
	A_HIS_PREV,             // Goto previous browsing history page
	A_HIS_NEXT,             // Goto next browsing history page
	A_TAB_ADD       = 200,  // Add new tab
	A_TAB_PREV,             // Switch to previous tab
	A_TAB_NEXT,             // Switch to next tab
	A_TAB_DUP,              // Duplicate current tab
	A_TAB_CLOSE,            // Close current tab
};

struct cmd {
	const char *name;
	enum action child;
};

#define CMD_ROOT A_QUIT
static struct cmd cmd_tree[] = {
	[A_QUIT]        = { "q: Quit program", 0 },
	[A_HELP]        = { "h: Help", 0 },
	[A_REPEAT]      = { ".: Repeat last command", 0 },
	[A_CANCEL]      = { "-: Cancel command", 0 },
	                  { "p: Page of current tab", A_PAGE_RELOAD },
	                  { "t: Tabs navigation", A_TAB_ADD },
	                  {0},
	[A_PAGE_RELOAD] = { "p: Reload current page", 0 },
	[A_PAGE_BODY]   = { "b: Print raw page response body", 0 },
	[A_HIS_LIST]    = { "h: List page history", 0 },
	[A_HIS_PREV]    = { "b: History: go back", 0 },
	[A_HIS_NEXT]    = { "f: History: go forward", 0 },
	                  { "-: Cancel command", CMD_ROOT },
	                  {0},
	[A_TAB_ADD]     = { "t: Add new tab", 0 },
	[A_TAB_PREV]    = { "p: Goto previous", 0 },
	[A_TAB_NEXT]    = { "n: Goto next", 0 },
	[A_TAB_DUP]     = { "d: Duplicat current tab", 0 },
	[A_TAB_CLOSE]   = { "c: Close current tab", 0 },
	                  { "-: Cancel command", CMD_ROOT },
	                  {0},
};

//
static enum action
cmd_action(struct cmd *cmd, char buf[BSIZ])
{
	size_t i, c=CMD_ROOT, b=0;
	assert(cmd);
	assert(buf);
	while (1) {
		for (; buf[b] >= ' '; b++) {
			while (cmd[c].name && cmd[c].name[0] != buf[b]) c++;
			if (!cmd[c].name) {
				return A_URI;
			}
			if (cmd[c].child > 0) {
				c = cmd[c].child;
				continue;
			}
			if (buf[b+1] >= ' ') {
				return A_URI;
			}
			return c;
		}
		for (i = c; cmd[i].name; i++) {
			printf("%8s%s\n",
			       cmd[i].child ? "+ " : "",
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
