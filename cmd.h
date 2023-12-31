/* Prompt commands. */

enum action {                   /* Possible action to input in prompt */
	A_NUL = 0,              /* Empty action */
	/* Default actions */
	A_URI,                  /* Absolute URI string */
	A_LINK,                 /* Index to link on current page */
	/* Prompt cmd */
	A_PAGE_RELOAD,          /* Reload current page */
	A_PAGE_RAW,             /* Show raw response */
	A_PAGE_HISTORY_PREV,    /* Goto previous browsing history page */
	A_PAGE_HISTORY_NEXT,    /* Goto next browsing history page */
	A_TAB_NEW,              /* Create new tab */
	A_TAB_PREV,             /* Switch to previous tab */
	A_TAB_NEXT,             /* Switch to next tab */
	A_TAB_DUPLICATE,        /* Duplicate current tab */
	A_TAB_CLOSE,            /* Close current tab */
	A_QUIT                  /* Exit program */
};

struct cmd {
	char c, *str;
	enum action action;
	struct cmd *child;
};

static struct cmd cmd_tree[] = {
	{ 'q', "quit", A_QUIT, 0 },
	{ 'p', "page", 0,
	  (struct cmd[]) {
		  { 'r', "reload", A_PAGE_RELOAD, 0 },
		  { 'R', "raw",    A_PAGE_RAW, 0 },
		  { 'h', "history", 0,
		    (struct cmd[]) {
			    { 'p', "previous", A_PAGE_HISTORY_PREV, 0 },
			    { 'n', "next",     A_PAGE_HISTORY_NEXT, 0 },
			    {0}
		    }
		  },
		  {0},
	  }
	},
	{ 't', "tab", 0,
	  (struct cmd[]) {
		  { 'N', "new",       A_TAB_NEW, 0 },
		  { 'p', "previous",  A_TAB_PREV, 0 },
		  { 'n', "next",      A_TAB_NEXT, 0 },
		  { 'd', "duplicate", A_TAB_DUPLICATE, 0 },
		  { 'c', "close",     A_TAB_CLOSE, 0 },
		  {0},
	  }
	},
	{0},
};

/**/
static void
cmd_print(struct cmd *cmd, char *path, int len)
{
	size_t i;
	printf("%.*s\n", len, path);
	for (i = 0; cmd[i].c; i++) {
		printf("\t%c: %s%s\n", cmd[i].c, cmd[i].str,
		       cmd[i].child ? " >" : "");
	}
}

/**/
static enum action
cmd_action(char *path)
{
	struct cmd *cmd = cmd_tree;
	int i, j;
	if (!path || !path[0]) {
		cmd_print(cmd, path, 0);
		return 0;
	}
	for (i = 0; path[i]; i++) {
		for (j = 0; cmd[j].c && cmd[j].c != path[i]; j++);
		if (cmd[j].c != path[i]) {
			return -1;
		}
		if (cmd[j].child) {
			cmd = cmd[j].child;
			continue;
		}
		if (!path[i+1]) {
			return cmd[j].action;
		}
	}
	cmd_print(cmd, path, i);
	return 0;
}
