#include "util.h"
#include "tab.h"
#include "le.h"

// Return TAB node under INDEX, NULL if not found.
static struct tab_node *
get_node(struct tab *tab, int index)
{
	struct tab_node *node;
	assert(tab);
	if (!tab->head || index < 0 || index >= tab->n) {
		return 0;
	}
	for (node = tab->head; index-- && node; node = node->next);
	return node;
}

// Return TAB node that points at NEXT node, NULL if not found.
static struct tab_node *
get_prev(struct tab *tab, struct tab_node *next)
{
	struct tab_node *node;
	assert(tab);
	if (!tab->head || !next || tab->head == next) {
		return 0;
	}
	for (node = tab->head; node->next != next; node = node->next);
	return node;
}

void
tab_new(struct tab *tab)
{
	struct tab_node *new;
	assert(tab);
	if (!(new = malloc(sizeof(*new)))) {
		WARN("malloc:");
		return;
	}
	memset(new, 0, sizeof(*new));
	tab->n++;
	if (!tab->head || !tab->open) {
		tab->head = new;
		tab->open = new;
		return;	
	}
	if (tab->open->next) {
		new->next = tab->open->next;
	}
	tab->open->next = new;
}

void
tab_open(struct tab *tab)
{
	tab_new(tab);
	tab_goto(tab, tab->i+1);
}

void
tab_goto(struct tab *tab, int index)
{
	struct tab_node *node;
	assert(tab);
	if (!(node = get_node(tab, index))) {
		return;
	}
	tab->open = node;
	tab->i = index;
}

void
tab_close(struct tab *tab, int index)
{
	struct tab_node *node, *prev;
	assert(tab);
	if (!(node = get_node(tab, index))) {
		return;
	}
	if ((prev = get_prev(tab, node))) {
		prev->next = node->next;
	} else {
		tab->head = node->next;
	}
	free(node);
	if (!--tab->n) {
		tab->open = 0;
		return;
	}
	if (index == tab->i) {
		index = CLAMP(0, index, tab->n-1);
		tab_goto(tab, index);
	}
}

void
tab_print(struct tab *tab)
{
	struct tab_node *node;
	int i;
	assert(tab);
	for (i = 1, node = tab->head; node; node = node->next, i++) {
		printf("\t%d: %s%s\n", i,
		       i == tab->i+1 ? "> " : "  ",
		       node->history[node->hi % TAB_HSIZ]);
	}
}

void
tab_history_add(struct tab_node *node, char *uri)
{
	assert(node);
	assert(uri);
	if (node->history[node->hi % TAB_HSIZ][0]) {
		node->hi++;
	}
	strncpy(node->history[node->hi % TAB_HSIZ], uri, URI_SIZ);
	// Make next history item empty to cut off old forward history
	// every time the new item is being added.
	node->history[(node->hi + 1) % TAB_HSIZ][0] = 0;
}

char *
tab_history_get(struct tab_node *node, int offset)
{
	assert(node);
	if (offset == 0) {
		return node->history[node->hi % TAB_HSIZ];
	}
	// TODO(irek): I expect that there is a bug when you loop over
	// the HSIZ and then try to get back.  I'm not checking for
	// the empty history entry.  Also when going forward I'm
	// looking only at very next entry but value of OFFSET can be
	// more then 1.
	if (offset > 0 && !node->history[(node->hi + 1) % TAB_HSIZ][0]) {
		return 0;
	}
	if (offset < 0 && !node->hi) {
		return 0;
	}
	node->hi += offset;
	return node->history[node->hi % TAB_HSIZ];
}
