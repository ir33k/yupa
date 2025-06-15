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
gmi_print(char *res, FILE *out)
{
	char *line, *word, *label, *env;
	unsigned i, n, w, maxw=0;

	env = getenv("YUPAWIDTH");
	if (env)
		maxw = atoi(env);

	if (maxw <= 10)
		maxw = 76;

	/* Skip header line */
	eachline(&res);

	while ((line = eachline(&res))) {
		if (!strncmp(line, "=>", 2)) {
			line = triml(line +2);
			label = linklabel(line);
			n = strcspn(line, "\t ");
			line[n] = 0;
			i = link_store(line);
			fprintf(out, "%-*u %s\n", MARGIN-1, i, label ? label : line);
			continue;
		}

		fprintf(out, "%-*s", MARGIN, "");
		w = MARGIN;
		while ((word = eachword(&line))) {
			n = strlen(word) +1;
			if (w + n > maxw) {
				fprintf(out, "\n");
				fprintf(out, "%-*s", MARGIN, "");
				w = MARGIN;
			}
			fprintf(out, "%s ", word);
			w += n;
		}
		fprintf(out, "\n");
	}
}
