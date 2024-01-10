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
	SH   = 50,              // Page shell commands execution
};

struct nav {                    // Navigation tree array item
	const int next;         // Action enum or index to nav menu
	// First char in NAME is command key and when second char
	// is '+' then CMD is used as index to submenu in array.
	const char *name;       // Action or submenu name
};

//
static const struct nav tree[] = {
[ROOT]=	{CMD_QUIT,      "q  Quit program"},
	{CMD_HELP,      "h  Print help message"},
	{PAGE,          "p+ Page commands"},
	{TAB,           "t+ Tabs commands"},
	{SH,            "!+ Shell commands"},
	{CMD_REPEAT,    ".  Repeat last command"},
	{CMD_CANCEL,    ",  Cancel command"},
	{0},	        
[PAGE]=	{CMD_PAGE_GET,  "r  Reload page"},
	{CMD_PAGE_RAW,  "b  Show raw response body"},
	{CMD_PAGE_URI,  "u  Print current page URI"},
	{HIST,          "h+ History commands"},
	{GET,           "d+ Download commands"},
	{ROOT,          ",+ Cancel command"},
	{0},	        
[HIST]=	{CMD_HIS_LIST,  "h  List page history entries"},
	{CMD_HIS_PREV,  "b  Go back in browsing history"},
	{CMD_HIS_NEXT,  "f  Go forward in browsing history"},
	{PAGE,          ",+ Cancel command"},
	{0},	        
[GET]=	{CMD_GET_RAW,   "d<path>  Download raw page"},
	{CMD_GET_FMT,   "f<path>  Download formatted page"},
	{PAGE,          ",+ Cancel command"},
	{0},	        
[TAB]=	{CMD_TAB_GOTO,  "t[index]  List tabs or goto tab by index"},
	{CMD_TAB_CLOSE, "c[index]  Close current tab or by index"},
	{CMD_TAB_OPEN,  "o[uri/link]  Clone tab or open new with URI/LINK"},
	{CMD_TAB_ADD,   "a  Add new empty tab"},
	{CMD_TAB_PREV,  "p  Go to previous tab"},
	{CMD_TAB_NEXT,  "n  Go to next tab"},
	{ROOT,          ",+ Cancel command"},
	{0},
[SH]=	{CMD_SH_RAW,    "!<cmd>  Run shell CMD on raw page"},
	{CMD_SH_FMT,    "f<cmd>  Run shell CMD on formatted page"},
	{ROOT,          ",+ Cancel command"},
	{0},	        
};

enum cmd
nav_cmd(char buf[BUFSIZ], char **arg)
{
	size_t i, c=0, b=0;
	assert(buf);
	*arg = 0;
	if (atoi(buf)) {
		return CMD_LINK;
	}
	if (uri_protocol(buf)) {
		return CMD_URI;
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
			while (buf[++b] && buf[b] <= ' ');
			*arg = buf[b] ? buf + b : 0;
			return tree[c].next;    // Found cmd
		}
		for (i = c; tree[i].name; i++) {
			printf("\t%s\n", tree[i].name);
		}
		if (BUFSIZ - b <= 0) {
			return 0;
		}
		printf("nav> %.*s", (int)b, buf);
		fflush(stdout);
		fgets(buf+b, BUFSIZ-b, stdin);
	}
	assert(0 && "unreachable");
}
