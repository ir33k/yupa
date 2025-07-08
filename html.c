#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "html.h"

enum element {
	E_NULL=0, E_A, E_ADDRESS, E_AREA, E_ARTICLE, E_ASIDE, E_B,
	E_BASE, E_BLOCKQUOTE, E_BR, E_BUTTON, E_CODE, E_COL, E_DD,
	E_DETAILS, E_DIV, E_DL, E_DT, E_EM, E_EMBED, E_FIELDSET,
	E_FIGCAPTION, E_FIGURE, E_FOOTER, E_FORM, E_H1, E_H2, E_H3,
	E_H4, E_H5, E_H6, E_HEADER, E_HR, E_I, E_IMG, E_INPUT, E_LI,
	E_LINK, E_MAIN, E_META, E_NAV, E_NOSCRIPT, E_OL, E_P, E_PARAM,
	E_PRE, E_SECTION, E_SELECT, E_SOURCE, E_SPAN, E_STRONG,
	E_TABLE, E_TD, E_TEXT, E_TFOOT, E_TH, E_THEAD, E_TR, E_TRACK,
	E_U, E_UL, E_VIDEO, E_WBR };

enum flag {
	F_INLINE = 1 << 0,	/* Display block is the default */
	F_VOID   = 1 << 1,	/* Self closed, with no children */
};

struct dynamic {
	void *pt;
	size_t item, capacity;
	unsigned n;
};

struct node {
	int parent, child, next;
	enum element type;
	char *title, *value;
};

#if 0

static const struct {
	char *name;
	unsigned flag;
} elements[] = {
	[E_NULL]        = { "",           0 },
	[E_ADDRESS]     = { "address",    0 },
	[E_AREA]        = { "area",       F_INLINE | F_VOID },
	[E_ARTICLE]     = { "article",    0 },
	[E_ASIDE]       = { "aside",      0 },
	[E_A]           = { "a",          F_INLINE },
	[E_BASE]        = { "base",       F_INLINE | F_VOID },
	[E_BLOCKQUOTE]  = { "blockquote", 0 },
	[E_BR]          = { "br",         F_INLINE | F_VOID },
	[E_BUTTON]      = { "button",     F_INLINE },
	[E_B]           = { "b",          F_INLINE },
	[E_CODE]        = { "code",       F_INLINE },
	[E_COL]         = { "col",        F_INLINE | F_VOID },
	[E_DD]          = { "dd",         0 },
	[E_DETAILS]     = { "details",    F_INLINE },
	[E_DIV]         = { "div",        0 },
	[E_DL]          = { "dl",         0 },
	[E_DT]          = { "dt",         0 },
	[E_EMBED]       = { "embed",      F_INLINE | F_VOID },
	[E_EM]          = { "em",         F_INLINE },
	[E_FIELDSET]    = { "fieldset",   0 },
	[E_FIGCAPTION]  = { "figcaption", 0 },
	[E_FIGURE]      = { "figure",     0 },
	[E_FOOTER]      = { "footer",     0 },
	[E_FORM]        = { "form",       0 },
	[E_H1]          = { "h1",         0 },
	[E_H2]          = { "h2",         0 },
	[E_H3]          = { "h3",         0 },
	[E_H4]          = { "h4",         0 },
	[E_H5]          = { "h5",         0 },
	[E_H6]          = { "h6",         0 },
	[E_HEADER]      = { "header",     0 },
	[E_HR]          = { "hr",         F_VOID },
	[E_IMG]         = { "img",        F_INLINE | F_VOID },
	[E_INPUT]       = { "input",      F_INLINE | F_VOID },
	[E_I]           = { "i",          F_INLINE },
	[E_LINK]        = { "link",       F_INLINE | F_VOID },
	[E_LI]          = { "li",         0 },
	[E_MAIN]        = { "main",       0 },
	[E_META]        = { "meta",       F_INLINE | F_VOID },
	[E_NAV]         = { "nav",        0 },
	[E_NOSCRIPT]    = { "noscript",   0 },
	[E_OL]          = { "ol",         0 },
	[E_PARAM]       = { "param",      F_INLINE | F_VOID },
	[E_PRE]         = { "pre",        0 },
	[E_P]           = { "p",          0 },
	[E_SECTION]     = { "section",    0 },
	[E_SELECT]      = { "select",     F_INLINE },
	[E_SOURCE]      = { "source",     F_INLINE | F_VOID },
	[E_SPAN]        = { "span",       F_INLINE },
	[E_STRONG]      = { "strong",     F_INLINE },
	[E_TABLE]       = { "table",      0 },
	[E_TD]          = { "td",         F_INLINE },
	[E_TEXT]        = { "text",       F_INLINE },
	[E_TFOOT]       = { "tfoot",      0 },
	[E_THEAD]       = { "thead",      F_INLINE },
	[E_TH]          = { "th",         F_INLINE },
	[E_TRACK]       = { "track",      F_INLINE | F_VOID },
	[E_TR]          = { "tr",         F_INLINE },
	[E_UL]          = { "ul",         0 },
	[E_U]           = { "u",          F_INLINE },
	[E_VIDEO]       = { "video",      0 },
	[E_WBR]         = { "wbr",        F_INLINE | F_VOID },
};

static struct dynamic nodes;

static void dynamic_init(struct dynamic *, size_t item, unsigned n);
static void dynamic_add(struct dynamic *, unsigned n);
static void dynamic_append(struct dynamic *, void *buf, unsigned n);
static void dynamic_clear(struct dynamic *);

void
dynamic_init(struct dynamic *d, size_t item, unsigned n)
{
	memset(d, 0, sizeof(*d));
	d->item = item;
	dynamic_add(d, n);
}

void
dynamic_add(struct dynamic *d, unsigned n)
{
	d->capacity += n;
	d->pt = realloc(d->pt, d->capacity * d->item);

	if (!d->pt)
		err(1, "dynamic realloc");
}

void
dynamic_append(struct dynamic *d, void *buf, unsigned n)
{
	if (d->capacity - d->n < n)
		dynamic_add(d, n);

	memcpy((char*)d->pt + d->n*d->item, buf, n*d->item);
	d->n += n;
}

void
dynamic_clear(struct dynamic *d)
{
	d->n = 0;
}

static char *parse_node_text(char *str, int parent);
static char *parse_node(char *str, int parent, int child);
static void parse(char *str);


char *
parse_node_text(char *str, int parent)
{
	struct node node = {0};

	node.parent = parent;
	node.type = E_TEXT;
	node.value = str;

	dynamic_append(&nodes, &node, 1);

	while (*str && *str != '<') str++;

	if (!*str)
		return str;

	*str = 0;
	return str +1;
}

char *
parse_node(char *str, int parent, int child)
{
	struct node node = {0};
	char *name, c;
	int i;

	nodes.pt[parent].child = child;
	node.parent = parent;

	str = str_trim_left(str);
	if (!*str)
		return;

	if (*str != '<')
		str = parse_node_text(str, parent);

	if (!*str)
		return;

	/* Ignore <!DOCUMENT html> */
	if (*str == '!')
		parse_node(str, parent);

	if (*str == '/')
		return;

	name = str;
	str = strcspn(str, "/> \t");
	if (!str) return;	/* Missing end of element name */
	c = *str;
	*str = 0;
	str++;

	for (i=1; i<COUNT(elements); i++) {
		if (!strncasecmp(name, elements[i].name)) {
			node.type = i;
			break;
		}
	}

	if (!node.type)
		node.value = name;

	if (c == ' ' || c == '\t') {
		str = str_trim_left(str);
		c = *str;
	}

	if (c != '/' && c != '>') {
	}
}

void
parse(char *str)
{
	struct node node = {0};

	node.type = E_NULL;
	node.value = "root";

	dynamic_append(&nodes, &node, 1);
	dynamic_append(&nodes, &node, 1);

	while (*str)
		str = parse_node(str, 0, nodes.n);
}
#endif

void
html_print(FILE *res, FILE *out)
{
	(void)res;
	(void)out;
}
