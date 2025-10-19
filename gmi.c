#include <err.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "gmi.h"

static int	utf_len		(char*);
static void	printwrap	(char *line, char *prefix, FILE *out);
static void	underline	(char *line, char mark, FILE *out);
static void	emit_link	(char *line, FILE *out);
static void	emit_li		(char *line, FILE *res, FILE *out);
static void	emit_pre	(char *line, FILE *res, FILE *out);

int
utf_len(char *str)
{
	int i;

	for (i=0; *str; str++)
		if (((*str) & 0xC0) != 0x80)
			i++;

	return i;
}

void
printwrap(char *str, char *prefix, FILE *out)
{
	char *word;
	int n, w, indent;

	fprintf(out, "%-*s%s", envmargin, "", prefix);

	str = trim(str);
	indent = strlen(prefix);
	w = envmargin;

	while ((word = eachword(&str))) {
		n = utf_len(word) +1;	/* +1 for space after word */
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
	int n, max;

	str = trim(str);
	n = utf_len(str);
	max = envwidth - envmargin;

	if (n > max)
		n = max;

	fprintf(out, "\n");
	printwrap(str, "", out);
	fprintf(out, "%-*s", envmargin, "");

	while (n--)
		fprintf(out, "%c", mark);

	fprintf(out, "\n");
}	

void
emit_link(char *line, FILE *out)
{
	char *label, prefix[16];
	unsigned i, n;

	line = trim(line);
	n = strcspn(line, "\t ");
	label = line[n] ? trim(line +n) : 0;
	line[n] = 0;
	i = link_store(line);

	snprintf(prefix, sizeof prefix, "%u> ", i);
	fprintf(out, "%-*s%s\n", envmargin, prefix, label ? label : line);
}

void
emit_li(char *line, FILE *res, FILE *out)
{
	char buf[4096], prefix[16];
	unsigned i;

	line = trim(line);

	/* NOTE(irek): Gemini supports only unordered list because
	 * that syntax is simpler.  Ordered lists are more useful,
	 * numbers help refer to specific points.  So I'm turning
	 * unordered list into ordered list.  Sue me. */

	/* NOTE(irek): It's possible that someone made ordered list by
	 * hand prefixing each list item with a number.  In that case
	 * it would look very ugly if we had numbers twice.  So if
	 * list item line starts with a digit then I assume that going
	 * with ordered list is a bad idea.  It would be perfect to
	 * check all list items but for simplicity I'm looking only at
	 * first line. */
	i = isdigit(line[0]) ? 0 : 1;

	while (1) {
		if (i) {	/* Ordered list */
			snprintf(prefix, sizeof prefix, "%u) ", i++);
			printwrap(line, prefix, out);
		} else {	/* Unordered list */
			printwrap(line, "* ", out);
		}

		line = fgets(buf, sizeof buf, res);

		if (!line || strncmp(line, "* ", 2)) {
			if (line && fseek(res, -strlen(line), SEEK_CUR) == -1)
				err(1, "gmi Listitem fseek");
			break;
		}

		line = trim(line +2);
	}
}

void
emit_pre(char *line, FILE *res, FILE *out)
{
	char buf[4096];

	fprintf(out, "%-*s```%s", envmargin, "", line);

	while ((line = fgets(buf, sizeof buf, res))) {
		if (!strcmp(line, "```\n"))
			break;

		fprintf(out, "%-*s%s", envmargin, "", line);
	}
	
	fprintf(out, "%-*s```\n", envmargin, "");
}

char *
gmi_search(char *header)
{
	static char buf[4096];

	if (header[0] != '1')
		return 0;

	/* TODO(irek): For response code 10 a regular text input is
	 * expected but for 11 a sensitive input like password should
	 * be taken without printing typed text in terminal. */

	printf("%s: ", header+3);
	fgets(buf+1, (sizeof buf) -1, stdin);
	trim(buf+1);
	buf[0] = '?';

	return buf;
}

Why
gmi_onheader(FILE *res, char **header, char **redirect)
{
	static char buf[4096];

	if (!fgets(buf, sizeof buf, res))
		return "Missing header";

	trim(buf);

	switch (buf[0]) {
	case '1':			/* input */
		/* This should be handled by calling gmi_search() */
		return 0;
	case '2':			/* ok */
		*header = buf + 3;
		return 0;
	case '3':			/* redirection */
		/* 30 Temporary redirection */
		/* 31 Permanent redirection */
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
	char buf[4096];

	/* TODO(irek): Using fixed size buffer is not ideal as in gemtext
	 * single line can be very long. */

	while (fgets(buf, sizeof buf, res)) {
		/**/ if (starts(buf, "=>"))	emit_link(buf+2, out);
		else if (starts(buf, "# "))	underline(buf+2, '=', out);
		else if (starts(buf, "## "))	underline(buf+3, '-', out);
		else if (starts(buf, "### "))	underline(buf+4, '.', out);
		else if (starts(buf, ">"))	printwrap(buf+1, "> ", out);
		else if (starts(buf, "* "))	emit_li(buf+2, res, out);
		else if (starts(buf, "```"))	emit_pre(buf+3, res, out);
		else /* Paragraph */		printwrap(buf, "", out);
	}
	fprintf(out, "\n");
}
