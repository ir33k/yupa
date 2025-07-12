#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <err.h>
#include "main.h"
#include "util.h"
#include "mime.h"
#include "link.h"
#include "html.h"

enum element {
	E_P, E_A, E_H1, E_H2, E_H3, E_H4, E_H5, E_H6, E_IMG, E_LI,
	E_UL, E_OL, E_PRE, E_BR, E_I, E_B, E_STRONG, E_U, E_EM, E_SMALL,
	E_DIV, E_SPAN, E_CODE, E_BLOCKQUOTE, E_DL, E_DT, E_HR, E_TD, E_TR,
	E_TH, E_TABLE, E_THEAD, E_TFOOT, E_HEADER, E_ASIDE, E_ARTICLE,
	E_SECTION, E_MAIN, E_FOOTER, E_NAV, E_FORM, E_INPUT, E_BUTTON,
	E_ADDRESS, E_AREA, E_COL, E_DD, E_DETAILS, E_EMBED,
	E_FIELDSET, E_FIGCAPTION, E_FIGURE, E_NOSCRIPT, E_PARAM,
	E_SELECT, E_SOURCE, E_TRACK, E_VIDEO, E_WBR, E_HEAD, E_TITLE,
	E_STYLE, E_SCRIPT, E_BASE, E_LINK, E_META, E_DOCTYPE, E_HTML,
	E_UNKNOWN};

enum attribute { A_NONE, A_UNKNOWN, A_HREF, A_SRC, A_TITLE };

enum flag {
	F_IGNORE = 1 << 0,
	F_BLOCK  = 1 << 1,	/* Display inline is the default */
	F_VOID   = 1 << 2,	/* Self closed, with no children */
};

static const struct {
	char *name;
	int flags;
} elements[] = {
	[E_P]           = { "p",          F_BLOCK },
	[E_A]           = { "a",          0 },
	[E_H1]          = { "h1",         F_BLOCK },
	[E_H2]          = { "h2",         F_BLOCK },
	[E_H3]          = { "h3",         F_BLOCK },
	[E_H4]          = { "h4",         F_BLOCK },
	[E_H5]          = { "h5",         F_BLOCK },
	[E_H6]          = { "h6",         F_BLOCK },
	[E_IMG]         = { "img",        F_VOID },
	[E_LI]          = { "li",         F_BLOCK },
	[E_UL]          = { "ul",         F_BLOCK },
	[E_OL]          = { "ol",         F_BLOCK },
	[E_PRE]         = { "pre",        F_BLOCK },
	[E_BR]          = { "br",         F_VOID },
	[E_I]           = { "i",          0 },
	[E_B]           = { "b",          0 },
	[E_STRONG]      = { "strong",     0 },
	[E_U]           = { "u",          0 },
	[E_EM]          = { "em",         0 },
	[E_SMALL]       = { "small",      0 },
	[E_DIV]         = { "div",        F_BLOCK },
	[E_SPAN]        = { "span",       0 },
	[E_CODE]        = { "code",       0 },
	[E_BLOCKQUOTE]  = { "blockquote", F_BLOCK },
	[E_DL]          = { "dl",         F_BLOCK },
	[E_DT]          = { "dt",         F_BLOCK },
	[E_HR]          = { "hr",         F_VOID },
	[E_TD]          = { "td",         0 },
	[E_TR]          = { "tr",         0 },
	[E_TH]          = { "th",         0 },
	[E_TABLE]       = { "table",      F_BLOCK },
	[E_THEAD]       = { "thead",      0 },
	[E_TFOOT]       = { "tfoot",      F_BLOCK },
	[E_HEADER]      = { "header",     F_BLOCK },
	[E_ASIDE]       = { "aside",      F_BLOCK },
	[E_ARTICLE]     = { "article",    F_BLOCK },
	[E_SECTION]     = { "section",    F_BLOCK },
	[E_MAIN]        = { "main",       F_BLOCK },
	[E_FOOTER]      = { "footer",     F_BLOCK },
	[E_NAV]         = { "nav",        F_BLOCK },
	[E_FORM]        = { "form",       F_BLOCK },
	[E_INPUT]       = { "input",      0 | F_VOID },
	[E_BUTTON]      = { "button",     0 },
	[E_ADDRESS]     = { "address",    F_BLOCK },
	[E_AREA]        = { "area",       0 | F_VOID },
	[E_COL]         = { "col",        0 | F_VOID },
	[E_DD]          = { "dd",         F_BLOCK },
	[E_DETAILS]     = { "details",    0 },
	[E_EMBED]       = { "embed",      0 | F_VOID },
	[E_FIELDSET]    = { "fieldset",   F_BLOCK },
	[E_FIGCAPTION]  = { "figcaption", F_BLOCK },
	[E_FIGURE]      = { "figure",     F_BLOCK },
	[E_NOSCRIPT]    = { "noscript",   F_BLOCK },
	[E_PARAM]       = { "param",      0 | F_VOID },
	[E_SELECT]      = { "select",     0 },
	[E_SOURCE]      = { "source",     0 | F_VOID },
	[E_TRACK]       = { "track",      0 | F_VOID },
	[E_VIDEO]       = { "video",      F_BLOCK },
	[E_WBR]         = { "wbr",        0 | F_VOID },
	[E_HEAD]        = { "head",       F_BLOCK },
	[E_TITLE]       = { "title",      F_BLOCK },
	[E_STYLE]       = { "style",      F_IGNORE },
	[E_SCRIPT]      = { "script",     F_IGNORE },
	[E_BASE]        = { "base",       F_BLOCK },
	[E_LINK]        = { "link",       F_IGNORE | F_VOID },
	[E_META]        = { "meta",       F_IGNORE | F_VOID },
	[E_DOCTYPE]     = { "!doctype",   F_IGNORE | F_VOID },
	[E_HTML]        = { "html",       F_BLOCK },
	[E_UNKNOWN]     = { "_unknown_",  F_BLOCK },
};

static struct {
	char *buf, *base;
	int w, block;
	unsigned link;
} ctx;

static char *fmalloc(FILE *);
static char *skipwhitespace(char *);
static void print(char *, FILE *out);
static enum attribute onattribute(char **str, char **value);
static void ontext(FILE *out);
static void onelement(FILE *out);

char *
fmalloc(FILE *fp)
{
	char *pt;
	long n, m;

	n = ftell(fp);

	if (fseek(fp, 0, SEEK_END))
		err(1, "html fmalloc fseek");

	m = ftell(fp) - n;

	if (!(pt = malloc(m +1)))
		err(1, "html fmalloc malloc(%ld)", m);

	if (fseek(fp, n, SEEK_SET))
		err(1, "html fmalloc fseek");

	n = fread(pt, 1, m, fp);
	pt[n] = 0;

	if (m != n)
		errx(1, "html fmalloc failed to read entire file");

	return pt;
}

char *
skipwhitespace(char *str)
{
	while (str && *str <= ' ') str++;
	return str;
}
void
print(char *str, FILE *out)
{
	char *word;
	int n;

	if (ctx.block) {
		fprintf(out, "\n\n");
		ctx.block = 0;
		ctx.w = 0;
	}

	if (ctx.w == 0) {
		fprintf(out, "%-*s", envmargin, "");
		ctx.w += envmargin;
	}

	while ((word = eachword(&str))) {
		n = strlen(word);
		if (n == 0)
			continue;

		if (ctx.w > envmargin) {
			if (ctx.w < envwidth)
				fprintf(out, " ");
			ctx.w++;
		}

		ctx.w += n;
		if (ctx.w > envwidth) {
			fprintf(out, "\n%-*s", envmargin, "");
			ctx.w = envmargin + n;
		}

		fprintf(out, "%s", word);
	}
}

enum attribute
onattribute(char **str, char **value)
{
	char *name, *pt, c;

	*value = 0;
	*str = skipwhitespace(*str);

	if (**str == '/' || **str == '>') return 0;	// No more
	name = *str;

	if (!(pt = strpbrk(*str, "=/>\0")))
		return 0;

	c = *pt;
	*str = *pt ? pt +1 : pt;
	*pt = 0;

	if (c == '=') {
		c = **str;

		if (c == '"')
			(*str)++;

		*value = *str;
		pt = strpbrk(*str, c == '"' ? "\0" : " \t\n\0/>");
		if (pt) {
			*str = *pt ? pt +1 : pt;
			*pt = 0;
		}
	}

	if (!strcasecmp(name, "href"))  return A_HREF;
	if (!strcasecmp(name, "src"))   return A_SRC;
	if (!strcasecmp(name, "title")) return A_TITLE;
	return A_UNKNOWN;
}

void
ontext(FILE *out)
{
	char *node, *pt;

	node = skipwhitespace(ctx.buf);
	if ((pt = strchr(node, '<'))) {
		*pt = 0;
		ctx.buf = pt +1;
	} else {
		ctx.buf = 0;
	}

	if (!*node)
		return;

	print(node, out);
}

void
onelement(FILE *out)
{
	char *node, *pt, *name, *value, buf[128];
	int closing, selfclosing, i, attr;
	unsigned n;

	if (!ctx.buf) return;

	node = skipwhitespace(ctx.buf);
	ctx.buf = strchr(node, '>');

	if (!ctx.buf) return;

	*ctx.buf = 0;
	ctx.buf++;

	name = node;
	closing = node[0] == '/';

	if (closing) {
		node++;
		name++;
	}

	// Terminate name
	if ((pt = strpbrk(name, " \n\t/>"))) {
		node = pt +1;
		*pt = 0;
	} else {
		node = "";
	}

	for (i=0; i<COUNT(elements); i++)
		if (!strcasecmp(elements[i].name, name))
			break;

	if (i == COUNT(elements))
		i = E_UNKNOWN;

	switch ((enum element)i) {
	case E_P: break;
	case E_A:
		if (closing) {
			snprintf(buf, sizeof buf, "][%d]", ctx.link);
			print(buf, out);
			break;
		}
		print("[", out);
		while ((attr = onattribute(&node, &value)))
			switch (attr) {
			case A_HREF:
				ctx.link = link_store(join(ctx.base, value ? value : ""));
				break;
			}
		break;
	case E_H1: break;
	case E_H2: break;
	case E_H3: break;
	case E_H4: break;
	case E_H5: break;
	case E_H6: break;
	case E_IMG: break;
	case E_LI: break;
	case E_UL: break;
	case E_OL: break;
	case E_PRE: break;
	case E_BR: break;
	case E_I: break;
	case E_B: break;
	case E_STRONG: break;
	case E_U: break;
	case E_EM: break;
	case E_SMALL: break;
	case E_DIV: break;
	case E_SPAN: break;
	case E_CODE: break;
	case E_BLOCKQUOTE: break;
	case E_DL: break;
	case E_DT: break;
	case E_HR: break;
	case E_TD: break;
	case E_TR: break;
	case E_TH: break;
	case E_TABLE: break;
	case E_THEAD: break;
	case E_TFOOT: break;
	case E_HEADER: break;
	case E_ASIDE: break;
	case E_ARTICLE: break;
	case E_SECTION: break;
	case E_MAIN: break;
	case E_FOOTER: break;
	case E_NAV: break;
	case E_FORM: break;
	case E_INPUT: break;
	case E_BUTTON: break;
	case E_ADDRESS: break;
	case E_AREA: break;
	case E_COL: break;
	case E_DD: break;
	case E_DETAILS: break;
	case E_EMBED: break;
	case E_FIELDSET: break;
	case E_FIGCAPTION: break;
	case E_FIGURE: break;
	case E_NOSCRIPT: break;
	case E_PARAM: break;
	case E_SELECT: break;
	case E_SOURCE: break;
	case E_TRACK: break;
	case E_VIDEO: break;
	case E_WBR: break;
	case E_HEAD: break;
	case E_TITLE: break;
	case E_STYLE: break;
	case E_SCRIPT: break;
	case E_BASE:
		while ((attr = onattribute(&node, &value)))
			switch (attr) {
			case A_HREF:
				ctx.base = value;
				break;
			}
		break;
	case E_LINK: break;
	case E_META: break;
	case E_DOCTYPE: break;
	case E_HTML: break;
	case E_UNKNOWN: break;
	}

	if (elements[i].flags & F_VOID)
		selfclosing = 1;

	// NOTE(irek): I have to consume all attributes no matter
	// the element type to make sure the last character before
	// '>' is not an attribute value.  There is an edge case:
	//
	//	<a href=/>
	//	        ^--- attribute value or selfclosing tag?
	//
	// If attributes with their values are not consumed it's
	// impossible to tell if '/' is a sign of selflosing tag.
	//
	while (onattribute(&node, &value));
	n = strlen(node);
	selfclosing = !selfclosing && !closing && node[n-1] == '/';

	(void)selfclosing;
	(void)out;

	if (elements[i].flags & F_BLOCK)
		ctx.block++;
}

void
html_print(FILE *res, FILE *out)
{
	char *buf;

	buf = fmalloc(res);

	memset(&ctx, 0, sizeof ctx);
	ctx.base = "";
	ctx.buf = buf;

	while (ctx.buf) {
		ontext(out);
		onelement(out);
	}

	fprintf(out, "\n");
	free(buf);
}

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
