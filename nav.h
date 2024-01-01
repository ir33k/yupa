// Prompt navigation for command actions

#ifndef _NAV_H
#define _NAV_H

#include <stdio.h>

enum nav_action {               // Possible action to input in nav prompt
	NAV_A_NUL = 0,          // Empty action
	NAV_A_URI,              // Absolute URI string (default action)
	NAV_A_LINK,             // Index to link on current page
	NAV_A_QUIT,             // Exit program
	NAV_A_HELP,             // Print program help
	NAV_A_SH_RAW,           // Run shell command on raw response body
	NAV_A_SH_FMT,           // Run shell command on formatted response
	NAV_A_REPEAT,           // Repeat last command
	NAV_A_CANCEL,           // Cancel insertion of current command
	NAV_A_PAGE_GET,         // Get (reload) current page
	NAV_A_PAGE_RAW,         // Show raw response body
	NAV_A_HIS_LIST,         // Print history list of current page
	NAV_A_HIS_PREV,         // Goto previous browsing history page
	NAV_A_HIS_NEXT,         // Goto next browsing history page
	NAV_A_TAB_GOTO,         // Goto taby by index / list tabs indexes
	NAV_A_TAB_ADD,          // Add new tab
	NAV_A_TAB_PREV,         // Switch to previous tab
	NAV_A_TAB_NEXT,         // Switch to next tab
	NAV_A_TAB_OPEN,         // Open current or provided page in new tab
	NAV_A_TAB_CLOSE,        // Close current tab
};

// Get action for BUF command string.  If BUF is empty or command
// incomplete then interactive prompt will ask for input as long as
// command is not completed.  If BUF contains number then assume LINK
// and if BUF starts with valid protocol name then assume URI.  ARG
// pointer will be set to point at beginning of command arguments in
// BUF if any.
enum nav_action nav_action(char buf[BUFSIZ], char **arg);

#endif // _NAV_H
