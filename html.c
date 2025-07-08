#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "util.h"
#include "mime.h"
#include "html.h"

enum {
	E_A, E_ADDRESS, E_AREA, E_ARTICLE, E_ASIDE, E_B,
	E_BASE, E_BLOCKQUOTE, E_BR, E_BUTTON, E_CODE, E_COL, E_DD,
	E_DETAILS, E_DIV, E_DL, E_DT, E_EM, E_EMBED, E_FIELDSET,
	E_FIGCAPTION, E_FIGURE, E_FOOTER, E_FORM, E_H1, E_H2, E_H3,
	E_H4, E_H5, E_H6, E_HEADER, E_HR, E_I, E_IMG, E_INPUT, E_LI,
	E_LINK, E_MAIN, E_META, E_NAV, E_NOSCRIPT, E_OL, E_P, E_PARAM,
	E_PRE, E_SECTION, E_SELECT, E_SOURCE, E_SPAN, E_STRONG,
	E_TABLE, E_TD, E_TEXT, E_TFOOT, E_TH, E_THEAD, E_TR, E_TRACK,
	E_U, E_UL, E_VIDEO, E_WBR};

enum flag {
	F_INLINE = 1 << 0,	/* Display block is the default */
	F_VOID   = 1 << 1,	/* Self closed, with no children */
};

static const struct {
	char *name;
	unsigned flag;
} elements[] = {
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

why_t
html_onheader(FILE *res, int *mime, char **redirect)
{
	char buf[4096], *pt;
	int code;

	if (!fgets(buf, sizeof buf, res))
		return "Missing HTTP header";

	if (!startswith(buf, "HTTP"))
		return "Missing HTTP header first line";

	if (!(pt = strchr(buf, ' ')))
		return "Missing HTTP response code";

	*mime = 0;
	*redirect = 0;
	code = atoi(trim(pt));

	switch (code) {
	case 200:	// Ok
		break;
	case 307:	// Temporary redirect
	case 308:	// Permanent redirect
		while (fgets(buf, sizeof buf, res)) {
			if (!strcmp(buf, "\r\n"))
				break;

			if (startswith(buf, "location:")) {
				*redirect = trim(buf +9);
				return 0;
			}
		}
		return "Failed to find locatoin header for redirection";
	default:
		return tellme("Failed with %s", trim(buf));
	}

	while (fgets(buf, sizeof buf, res)) {
		if (!strcmp(buf, "\r\n"))
			break;

		if (!*mime && startswith(buf, "content-type:"))
			*mime = mime_header(trim(buf +13));
	}

	return 0;
}

void
html_print(FILE *res, FILE *out)
{
	
	(void)res;
	(void)out;
}
