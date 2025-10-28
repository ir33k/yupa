#include <stdio.h>
#include <string.h>
#include "main.h"

/*
 * Do nothing to text file except of collecting URLs.
 */
void
txt_print(FILE *res, FILE *out)
{
	char buf[4096], *pt, *word;

	while ((pt = fgets(buf, sizeof buf, res))) {
		fprintf(out, "%s", pt);

		while ((word = eachword(&pt, " \t\n()<>[]")))
			if (strstr(word, "://"))
				link_store(word);
	}
}
