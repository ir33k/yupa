// Prompt commands.

#ifndef _CMD_H
#define _CMD_H

#include <stdio.h>

enum cmd_action {               // Possible action to input in cmd prompt
	CMD_A_NUL = 0,          // Empty action
	CMD_A_URI,              // Absolute URI string (default action)
	CMD_A_LINK,             // Index to link on current page
	CMD_A_QUIT,             // Exit program
	CMD_A_HELP,             // Print program help
	CMD_A_REPEAT,           // Repeat last command
	CMD_A_CANCEL,           // Cancel insertion of current command
	CMD_A_PAGE_GET,         // Get (reload) current page
	CMD_A_PAGE_BODY,        // Show response body
	CMD_A_HIS_LIST,         // Print history list of current page
	CMD_A_HIS_PREV,         // Goto previous browsing history page
	CMD_A_HIS_NEXT,         // Goto next browsing history page
	CMD_A_TAB_LIST,         // Print list of tabs
	CMD_A_TAB_ADD,          // Add new tab
	CMD_A_TAB_PREV,         // Switch to previous tab
	CMD_A_TAB_NEXT,         // Switch to next tab
	CMD_A_TAB_DUP,          // Duplicate current tab
	CMD_A_TAB_CLOSE,        // Close current tab
};

struct cmd {
	enum cmd_action action;
	// First char in NAME is command key and when fourth char is
	// '+' then ACTION is used as index to CMD array.
	const char *name;
};

extern const struct cmd cmd_tree[];

//
enum cmd_action cmd_action(const struct cmd *cmd, char buf[BUFSIZ], char **arg);

#endif // _CMD_H
