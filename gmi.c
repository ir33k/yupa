#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "link.h"
#include "gmi.h"

#define MARGIN 4

/*
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
static void Text	(char **, char *, FILE *);

/* gemini://geminiprotocol.net/docs/gemtext.gmi */
static struct {
	const char *prefix;
	printer_t printer;
} markup[] = {
	"=>",	Link,
	"* ",	Listitem,
	">",	Blockquote,
	"# ",	Heading1,
	"## ",	Heading2,
	"### ",	Heading3,
	"```",	Preformat,
};

char *
linklabel(char *line)
{
	unsigned n;
	n = strcspn(line, "\t ");

	if (!line[n])
		return 0;

	return triml(line +n);
}

void
printwrap(char *str, char *prefix, FILE *out)
{
	static int maxw=0;
	char *word, *env;
	int n, w, indent;

	if (!maxw) {
		env = getenv("YUPAMAXW");
		if (env)
			maxw = atoi(env);

		if (maxw <= 10)
			maxw = 76;
	}

	fprintf(out, "%-*s%s", MARGIN, "", prefix);

	indent = strlen(prefix);
	w = MARGIN;

	while ((word = eachword(&str))) {
		n = strlen(word) +1;
		if (w + n > maxw) {
			fprintf(out, "\n");
			fprintf(out, "%-*s", MARGIN + indent, "");
			w = MARGIN;
		}
		fprintf(out, "%s ", word);
		w += n;
	}
	fprintf(out, "\n");
}

void
underline(char *str, char mark, FILE *out)
{
	unsigned n;

	n = strlen(str);

	fprintf(out, "\n");
	fprintf(out, "%-*s%s\n", MARGIN, "", str);
	fprintf(out, "%-*s", MARGIN, "");

	while (n--)
		fprintf(out, "%c", mark);

	fprintf(out, "\n");
}	

void
Link(char **res, char *line, FILE *out)
{
	char *label;
	unsigned i, n;

	(void)res;

	label = linklabel(line);
	n = strcspn(line, "\t ");
	line[n] = 0;
	i = link_store(line);

	fprintf(out, ":%-*u %s\n", MARGIN-2, i, label ? label : line);
}

void
Listitem(char **res, char *line, FILE *out)
{
	char prefix[16];
	unsigned i;

	(void)res;

	/* NOTE(irek): Gemini supports only unordered list but only
	 * because that syntax is simpler.  Ordered lists are more
	 * usefull, numbers help refer to specific point.  So I'm
	 * turning unordered list to ordered list.  Sue me. */

	/* NOTE(irek): It's possible that someone made ordered list by
	 * hand prefixing each list item with a number.  In that case
	 * it would look very ugly if we had numbers twice.  So if
	 * list item line starts with a digit then I assume that going
	 * with ordered list is a bad idea.  It would be perfect to
	 * check all list items but for simplicity I'm looking only at
	 * first line. */
	i = isdigit(*line) ? 0 : 1;

	while (1) {
		/* Unordered list */
		if (!i) {
			printwrap(line, "* ", out);
			break;
		}

		/* Ordered list */
		snprintf(prefix, sizeof prefix, "%u) ", i++);
		printwrap(line, prefix, out);

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
	(void)res;
	fprintf(out, "%-*s### %s\n", MARGIN, "", line);
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
Text(char **res, char *line, FILE *out)
{
	(void)res;
	printwrap(line, "", out);
}

void
gmi_print(char *res, FILE *out)
{
	char *line;
	unsigned i, n;

	/* Skip header line */
	eachline(&res);

	while ((line = eachline(&res))) {
		for (i=0; i<SIZE(markup); i++) {
			n = strlen(markup[i].prefix);
			if (!strncmp(line, markup[i].prefix, n)) {
				line = triml(line +n);
				(*markup[i].printer)(&res, line, out);
				break;
			}
		}

		if (i == SIZE(markup))
			Text(&res, line, out);
	}
}
