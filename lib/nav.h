// Prompt navigation for command actions.

#ifndef _NAV_H
#define _NAV_H

#include <stdio.h>

enum cmd {              // Possible commands input in nav prompt
	CMD_NUL = 0,    // Empty action
	CMD_URI,        // Absolute URI string (default action)
	CMD_LINK,       // Index to link on current page
	//
	CMD_QUIT,       // Exit program
	CMD_HELP,       // Print program help
	CMD_REPEAT,     // Repeat last command
	CMD_CANCEL,     // Cancel insertion of current command
	//
	CMD_PAGE_GET,   // Get (reload) current page
	CMD_PAGE_RAW,   // Show raw response body
	//
	CMD_HIS_LIST,   // Print history list of current page
	CMD_HIS_PREV,   // Goto previous browsing history page
	CMD_HIS_NEXT,   // Goto next browsing history page
	//
	CMD_GET_RAW,    // Download current page raw response
	CMD_GET_FMT,    // Download current page formatted response
	//
	CMD_TAB_GOTO,   // Goto taby by index or list tabs indexes
	CMD_TAB_ADD,    // Add new tab
	CMD_TAB_PREV,   // Switch to previous tab
	CMD_TAB_NEXT,   // Switch to next tab
	CMD_TAB_OPEN,   // Open current or provided page in new tab
	CMD_TAB_CLOSE,  // Close current tab
	//
	CMD_SH_RAW,     // Run shell command on raw response body
	CMD_SH_FMT,     // Run shell command on formatted response
};

// Get command for BUF command string.  If BUF is empty or command
// incomplete then interactive prompt will ask for input as long as
// command is not completed.  If BUF contains number then assume LINK
// and if BUF starts with valid protocol name then assume URI.  ARG
// pointer will be set to point at beginning of command arguments in
// BUF if any.
enum cmd nav_cmd(char buf[BUFSIZ], char **arg);

#endif // _NAV_H
