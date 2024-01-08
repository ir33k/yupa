// Tabs.

#ifndef _TAB_H
#define _TAB_H

#include <stdio.h>
#include "uri.h"
#include "past.h"

struct tab {
	// Readonly
	struct tab_node *head;  // Linked list head node
	struct tab_node *open;  // Currently open tab
	int              n, i;  // Number of tabs, index of open tab
};

struct tab_node {               // Tab node in liked list
	enum uri protocol;      // Current page URI protocol
	char    raw[FILENAME_MAX]; // Path to file with raw response
	char    fmt[FILENAME_MAX]; // Path to file with formatted RAW
	// Readonly
	struct past     *past;  // Browsing history ring buffer
	struct tab_node *next;  // Next linked list node
};

// Add new empty tab and set it as current tab.
void tab_new(struct tab *tab);

// Like tab_new but also sets new tab as open (active) tab.
void tab_open(struct tab *tab);

// Set tab node under INDEX as open (active) tab.
void tab_goto(struct tab *tab, int index);

// Sloce tab node under INDEX.
void tab_close(struct tab *tab, int index);

// Print list of all tabs.
void tab_print(struct tab *tab);

// Add new URI to current tab history.
void tab_history_add(struct tab_node *node, char *uri);

// Get current tab history item shifting history index by SHIFT.
char *tab_history_get(struct tab_node *node, int offset);

#endif // _TAB_H
