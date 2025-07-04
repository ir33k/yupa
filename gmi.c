#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "link.h"
#include "main.h"
#include "gmi.h"

/*
  	if (!strncmp(head+3, "text/gemini", 11)) {
		gmi_fmt(raw, fmt);
		return NET_FMT;
	}
	if (!strncmp(head+3, "text/", 5)) {
		return NET_RAW;
	}


From gmix/gmit.h project source file.
	GMIR_NUL          =  0, // Unknown status code
	                        // 1X INPUT
	GMIR_INPUT_TEXT   = 10, // Regular input, search phrase
	GMIR_INPUT_PASS   = 11, // Sensitive input, password
	                        // 2X SUCCESS
	GMIR_OK           = 20, // All good
	                        // 3X Redirection
	GMIR_REDIR_TEMP   = 30, // Temporary redirection
	GMIR_REDIR_PERM   = 31, // Permanent redirection
	                        // 4X TMP FAIL
	GMIR_WARN_TEMP    = 40, // Temporary failure
	GMIR_WARN_OUT     = 41, // Server unavailable
	GMIR_WARN_CGI     = 42, // CGI error
	GMIR_WARN_PROX    = 43, // Proxy error
	GMIR_WARN_LIMIT   = 44, // Rate limiting, you have to wait
	                        // 5X PERMANENT FAIL
	GMIR_ERR_PERM     = 50, // Permanent failure
	GMIR_ERR_404      = 51, // Not found
	GMIR_ERR_GONE     = 52, // Resource no longer available
	GMIR_ERR_NOPROX   = 53, // Proxy request refused
	GMIR_ERR_BAD      = 59, // Bad requaest
	                        // 6X CLIENT CERT
	GMIR_CERT_REQU    = 60, // Client certificate required
	GMIR_CERT_UNAUTH  = 61, // Certificate not authorised
	GMIR_CERT_INVALID = 62, // Cerfiticate not valid
*/

static char *linklabel(char *line);
static void printwrap(char *str, char *prefix, FILE *out);
static void underline(char *str, char mark, FILE *out);

typedef void (*printer_t)(char **res, char *line, FILE *out);

static void Link	(char **, char *, FILE *);
static void Listitem	(char **, char *, FILE *);
static void Blockquote	(char **, char *, FILE *);
static void Heading1	(char **, char *, FILE *);
static void Heading2	(char **, char *, FILE *);
static void Heading3	(char **, char *, FILE *);
static void Preformat	(char **, char *, FILE *);
static void Empty	(char **, char *, FILE *);
static void Text	(char **, char *, FILE *);

/* gemini://geminiprotocol.net/docs/gemtext.gmi */
static struct {
	const char *prefix;
	unsigned n;
	printer_t printer;
} markup[] = {
	"=>",	2, Link,
	"* ",	2, Listitem,
	">",	1, Blockquote,
	"# ",	2, Heading1,
	"## ",	3, Heading2,
	"### ",	4, Heading3,
	"```",	3, Preformat,
	"",	1, Empty,
	"",	0, Text,
};

char *
linklabel(char *line)
{
	unsigned n = strcspn(line, "\t ");
	return line[n] ? triml(line +n) : 0;
}

void
printwrap(char *str, char *prefix, FILE *out)
{
	char *word;
	int n, w, indent;

	fprintf(out, "%-*s%s", envmargin, "", prefix);

	indent = strlen(prefix);
	w = envmargin;

	while ((word = eachword(&str))) {
		n = strlen(word) +1;
		if (w + n > envwidth) {
			fprintf(out, "\n");
			fprintf(out, "%-*s", envmargin + indent, "");
			w = envmargin;
		}
		fprintf(out, "%s ", word);
		w += n;
	}
	fprintf(out, "\n");
}

void
underline(char *str, char mark, FILE *out)
{
	int n;

	n = strlen(str);

	if (n > (envwidth - envmargin))
		n = envwidth - envmargin;

	fprintf(out, "\n");
	printwrap(str, "", out);
	fprintf(out, "%-*s", envmargin, "");

	while (n--)
		fprintf(out, "%c", mark);

	fprintf(out, "\n");
}	

void
Link(char **res, char *line, FILE *out)
{
	char *label, prefix[16];
	unsigned i, n;

	(void)res;

	label = linklabel(line);
	n = strcspn(line, "\t ");
	line[n] = 0;
	i = link_store(line);

	snprintf(prefix, sizeof prefix, "%u> ", i);
	fprintf(out, "%-*s%s\n", envmargin, prefix, label ? label : line);
}

void
Listitem(char **res, char *line, FILE *out)
{
	char prefix[16];
	unsigned i;

	(void)res;

	/* NOTE(irek): Gemini supports only unordered list but only
	 * because that syntax is simpler.  Ordered lists are more
	 * useful, numbers help refer to specific points.  So I'm
	 * turning unordered list into ordered list.  Sue me. */

	/* NOTE(irek): It's possible that someone made ordered list by
	 * hand prefixing each list item with a number.  In that case
	 * it would look very ugly if we had numbers twice.  So if
	 * list item line starts with a digit then I assume that going
	 * with ordered list is a bad idea.  It would be perfect to
	 * check all list items but for simplicity I'm looking only at
	 * first line. */
	i = isdigit(*line) ? 0 : 1;

	while (1) {
		if (i) {	/* Ordered list */
			snprintf(prefix, sizeof prefix, "%u) ", i++);
			printwrap(line, prefix, out);
		} else {	/* Unordered list */
			printwrap(line, "* ", out);
		}

		if (strncmp(*res, "* ", 2))
			break;

		line = eachline(res);
		line = triml(line +2);
	}
}

void
Blockquote(char **res, char *line, FILE *out)
{
	(void)res;
	printwrap(line, "> ", out);
}

void
Heading1(char **res, char *line, FILE *out)
{
	(void)res;
	underline(line, '=', out);
}

void
Heading2(char **res, char *line, FILE *out)
{
	(void)res;
	underline(line, '-', out);
}

void
Heading3(char **res, char *line, FILE *out)
{
	char buf[4096];

	(void)res;

	snprintf(buf, sizeof buf, "%s ###", line);
	printwrap(buf, "### ", out);
}

void
Preformat(char **res, char *line, FILE *out)
{
	fprintf(out, "```%s\n", line);

	while ((line = eachline(res))) {
		if (!strcmp(line, "```"))
			break;

		fprintf(out, "%s\n", line);
	}
	
	fprintf(out, "```\n");
}

void
Empty(char **res, char *line, FILE *out)
{
	(void)line;

	fprintf(out, "\n");

	/* NOTE(irek): Avoid multiple empty lines. */
	while ((**res) == '\n')
		eachline(res);
}

void
Text(char **res, char *line, FILE *out)
{
	(void)res;
	printwrap(line, "", out);
}

void
gmi_print(char *res, FILE *out)
{
	char *line;
	unsigned i;

	/* Skip header line */
	eachline(&res);

	while ((line = eachline(&res))) {
		/* NOTE(irek): -1 is used to skip check for last
		 * markup.  With that last element becomes the default
		 * which is Text. */
		for (i=0; i<SIZE(markup)-1; i++)
			if (!strncmp(line, markup[i].prefix, markup[i].n))
				break;

		line = triml(line + strlen(markup[i].prefix));
		(*markup[i].printer)(&res, line, out);
	}
}
