/* Prompt commands. */

enum action {                   /* Possible action to input in prompt */
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
	/* Convention is that when STR starts with '+' char then NEXT
	 * is an index to child command group in cmd_tree, else it is
	 * an action. */
	int next;
};

static struct cmd cmd_tree[] = {
	{ 'p', "+page",     100 },
	{ 't', "+tab",      200 },
	{ 'h', "+history",  300 },
	{ 'q', "quit",      A_QUIT },
	{0},
[100] =	{ 'r', "reload",    A_PAGE_RELOAD },
	{ 'R', "raw",       A_PAGE_RAW },
	{0},
[200] =	{ 'N', "new",       A_TAB_NEW },
	{ 'p', "previous",  A_TAB_PREV },
	{ 'n', "next",      A_TAB_NEXT },
	{ 'd', "duplicate", A_TAB_DUPLICATE },
	{ 'c', "close",     A_TAB_CLOSE },
	{0},
[300] =	{ 'p', "previous",  A_HISTORY_PREV },
	{ 'n', "next",      A_HISTORY_NEXT },
	{0}
};

/**/
static void
cmd_print(size_t i, char *path, int len)
{
	printf("%.*s\n", len, path);
	for (; cmd_tree[i].c; i++) {
		printf("\t%c: %s\n", cmd_tree[i].c, cmd_tree[i].str);
	}
}

/**/
static enum action
cmd_action(char *path)
{
	int i, j;
	if (!path || !path[0]) {
		cmd_print(0, path, 0);
		return 0;
	}
	for (i = 0, j = 0; path[i]; i++) {
		for (; cmd_tree[j].c && cmd_tree[j].c != path[i]; j++);
		if (cmd_tree[j].c != path[i]) {
			return -1;
		}
		if (cmd_tree[j].str[0] == '+') {
			j = cmd_tree[j].next;
			continue;
		}
		if (!path[i+1]) {
			return cmd_tree[j].next;
		}
	}
	cmd_print(j, path, i);
	return 0;
}
