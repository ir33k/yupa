#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "txt.h"

/*
 * Do nothing to text file except of collecting URLs.
 */
void
txt_print(FILE *res, FILE *out)
{
	char buf[4096], *pt, *word;

	while ((pt = fgets(buf, sizeof buf, res))) {
		fprintf(out, "%s", pt);

		while ((word = eachword(&pt)))
			if (strstr(word, "://"))
				link_store(word);
	}
}
