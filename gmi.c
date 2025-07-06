#include <err.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "util.h"
#include "mime.h"
#include "link.h"
#include "gmi.h"

static char *linklabel(char *line);
static void printwrap(char *str, char *prefix, FILE *out);
static void underline(char *str, char mark, FILE *out);

typedef void (*printer_t)(char *line, FILE *res, FILE *out);

static void Link	(char *, FILE *, FILE *);
static void Listitem	(char *, FILE *, FILE *);
static void Blockquote	(char *, FILE *, FILE *);
static void Heading1	(char *, FILE *, FILE *);
static void Heading2	(char *, FILE *, FILE *);
static void Heading3	(char *, FILE *, FILE *);
static void Preformat	(char *, FILE *, FILE *);
static void Empty	(char *, FILE *, FILE *);
static void Text	(char *, FILE *, FILE *);

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
	return line[n] ? trim(line +n) : 0;
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
		n = strlen(word) +1;	// +1 for space after word
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
Link(char *line, FILE *res, FILE *out)
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
Listitem(char *line, FILE *res, FILE *out)
{
	char buf[4096], prefix[16];
	unsigned i;

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
	i = isdigit(line[0]) ? 0 : 1;

	while (1) {
		if (i) {	/* Unordered list */
			printwrap(line, "* ", out);
		} else {	/* Ordered list */
			snprintf(prefix, sizeof prefix, "%u) ", i++);
			printwrap(line, prefix, out);
		}

		line = fgets(buf, sizeof buf, res);

		if (!line || strncmp(line, "* ", 2)) {
			if (fseek(res, -strlen(line), SEEK_CUR) == -1)
				err(1, "gmi Listitem fseek");
			break;
		}

		line = trim(line +2);
	}
}

void
Blockquote(char *line, FILE *res, FILE *out)
{
	(void)res;
	printwrap(line, "> ", out);
}

void
Heading1(char *line, FILE *res, FILE *out)
{
	(void)res;
	underline(line, '=', out);
}

void
Heading2(char *line, FILE *res, FILE *out)
{
	(void)res;
	underline(line, '-', out);
}

void
Heading3(char *line, FILE *res, FILE *out)
{
	char buf[4096];

	(void)res;

	snprintf(buf, sizeof buf, "%s ###", line);
	printwrap(buf, "### ", out);
}

void
Preformat(char *line, FILE *res, FILE *out)
{
	char buf[4096];

	fprintf(out, "%-*s```%s\n", envmargin, "", line);

	while ((line = fgets(buf, sizeof buf, res))) {
		if (!strcmp(line, "```\n"))
			break;

		fprintf(out, "%-*s%s", envmargin, "", line);
	}
	
	fprintf(out, "%-*s```\n", envmargin, "");
}

void
Empty(char *line, FILE *res, FILE *out)
{
	(void)line;
	(void)res;
	fprintf(out, "\n");
}

void
Text(char *line, FILE *res, FILE *out)
{
	(void)res;
	printwrap(line, "", out);
}

char *
gmi_search(char *header)
{
	static char buf[4096];

	if (header[0] != '1')
		return 0;

	/* TODO(irek): For respnse code 10 a regular text input is
	 * expected but for 11 a sensitive input like password should
	 * be taken without printing typed text in terminal. */

	printf("%s: ", header+3);
	fgets(buf+1, (sizeof buf) -1, stdin);
	trim(buf+1);
	buf[0] = '?';

	return buf;
}

why_t
gmi_onheader(FILE *res, int *mime, char **redirect)
{
	static char buf[4096];

	if (!fgets(buf, sizeof buf, res))
		return "Missing header";

	trim(buf);

	switch (buf[0]) {
	case '1':			// input
		/* This should be handled by calling gmi_search() */
		return 0;
	case '2':			// ok
		*mime = mime_header(buf +3);
		return 0;
	case '3':			// redirection
		// 30 Temporary redirection
		// 31 Permanent redirection
		*redirect = buf +3;
		return 0;
	case '4':
		switch (buf[1]) {
		default:
		case '0': return "Temporary failure";
		case '1': return "Server unavailable";
		case '2': return "CGI error";
		case '3': return "Proxy error";
		case '4': return "Slow down, rate limiting, wait";
		}
	case '5':
		switch (buf[1]) {
		default:
		case '0': return "Permanent failure";
		case '1': return "Not found";
		case '2': return "Gone, no longer available";
		case '3': return "Proxy request refused";
		case '9': return "Bad request";
		}
	case '6':
		switch (buf[1]) {
		default:
		case '0': return "Client certificate required";
		case '1': return "Certificate not authorized";
		case '2': return "Certificate not valid";
		}
	}
	return "Unknown response code";
}

void
gmi_print(FILE *res, FILE *out)
{
	char buf[4096], *line;
	unsigned i;

	while ((line = fgets(buf, sizeof buf, res))) {
		/* NOTE(irek): -1 is used to skip check for last markup.
		 * With that last element it the default which is Text. */
		for (i=0; i<SIZE(markup)-1; i++)
			if (!strncmp(line, markup[i].prefix, markup[i].n))
				break;

		line = trim(line + strlen(markup[i].prefix));
		(*markup[i].printer)(line, res, out);
	}
	fprintf(out, "\n");
}
