/* Prompt commands. */

enum action {                   /* Possible actions to input in prompt */
	A_NUL = 0,              /* Empty action */
	/* Default actions */
	A_URI,                  /* Absolute URI string */
	A_LINK,                 /* Index to link on current page */
	/* Prompt cmd */
	A_PAGE_RELOAD,          /* Reload current page */
	A_PAGE_RAW,             /* Show raw response */
	A_TAB_NEW,              /* Create new tab */
	A_TAB_PREV,             /* Switch to previous tab */
	A_TAB_NEXT,             /* Switch to next tab */
	A_TAB_DUPLICATE,        /* Duplicate current tab */
	A_TAB_CLOSE,            /* Close current tab */
	A_HISTORY_PREV,         /* Goto previous browsing history page */
	A_HISTORY_NEXT,         /* Goto next browsing history page */
	A_QUIT                  /* Exit program */
};

struct cmd {
	char c, *str;
	/* Convention is that when STR starts with '+' char then union
	 * is a pointer to child array, else it is an action. */
	union {
		struct cmd *child;
		enum action action;
	} v;
};

static struct cmd cmd_page[] = {
	{ 'r', "reload",    { (struct cmd *)A_PAGE_RELOAD } },
	{ 'R', "raw",       { (struct cmd *)A_PAGE_RAW } },
	{0}
};

static struct cmd cmd_tab[] = {
	{ 'N', "new",       { (struct cmd *)A_TAB_NEW } },
	{ 'p', "previous",  { (struct cmd *)A_TAB_PREV } },
	{ 'n', "next",      { (struct cmd *)A_TAB_NEXT } },
	{ 'd', "duplicate", { (struct cmd *)A_TAB_DUPLICATE } },
	{ 'c', "close",     { (struct cmd *)A_TAB_CLOSE } },
	{0}
};

static struct cmd cmd_history[] = {
	{ 'p', "previous",  { (struct cmd *)A_HISTORY_PREV } },
	{ 'n', "next",      { (struct cmd *)A_HISTORY_NEXT } },
	{0}
};

static struct cmd cmd_root[] = {
	/* Parents */
	{ 'p', "+page",     { cmd_page } },
	{ 't', "+tab",      { cmd_tab } },
	{ 'h', "+history",  { cmd_history } },
	/* Actions */
	{ 'q', "quit",      { (struct cmd *)A_QUIT } },
	{0}
};

static void
cmd_print(struct cmd *cmd)
{
	size_t i;
	for (i = 0; cmd[i].c; i++) {
		printf("\t%c: %s\n", cmd[i].c, cmd[i].str);
	}
}

static enum action
cmd_action(char *path)
{
	size_t i;
	struct cmd *cmd = cmd_root;
	if (!path || !path[0]) {
		cmd_print(cmd);
		return 0;
	}
	for (; *path; path++) {
		for (i = 0; cmd[i].c && cmd[i].c != *path; i++);
		if (cmd[i].c != *path) {
			return -1;
		}
		if (cmd[i].str[0] == '+') {
			cmd = cmd[i].v.child;
			continue;
		}
		if (!path[1]) {
			return cmd[i].v.action;
		}
	}
	cmd_print(cmd);
	return 0;
}
