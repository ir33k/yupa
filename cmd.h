// Prompt action commands.

#ifndef _CMD_H
#define _CMD_H

#include <stdio.h>

enum cmd_action {               // Possible action to input in cmd prompt
	CMD_A_NUL = 0,          // Empty action
	CMD_A_URI,              // Absolute URI string (default action)
	CMD_A_LINK,             // Index to link on current page
	CMD_A_QUIT,             // Exit program
	CMD_A_HELP,             // Print program help
	CMD_A_SH_RAW,           // Run shell command on raw response body
	CMD_A_SH_FMT,           // Run shell command on formatted response
	CMD_A_REPEAT,           // Repeat last command
	CMD_A_CANCEL,           // Cancel insertion of current command
	CMD_A_PAGE_GET,         // Get (reload) current page
	CMD_A_PAGE_RAW,         // Show raw response body
	CMD_A_HIS_LIST,         // Print history list of current page
	CMD_A_HIS_PREV,         // Goto previous browsing history page
	CMD_A_HIS_NEXT,         // Goto next browsing history page
	CMD_A_TAB_GOTO,         // Goto taby by index / list tabs indexes
	CMD_A_TAB_ADD,          // Add new tab
	CMD_A_TAB_PREV,         // Switch to previous tab
	CMD_A_TAB_NEXT,         // Switch to next tab
	CMD_A_TAB_OPEN,         // Open current or provided page in new tab
	CMD_A_TAB_CLOSE,        // Close current tab
};

// Get action for BUF command string.  If BUF is empty or command
// incomplete then interactive prompt will ask for input as long as
// command is not completed.  If BUF contains number then assume LINK
// and if BUF starts with valid protocol name then assume URI.  ARG
// pointer will be set to point at beginning of command arguments in
// BUF if any.
enum cmd_action cmd_action(char buf[BUFSIZ], char **arg);

#endif // _CMD_H
