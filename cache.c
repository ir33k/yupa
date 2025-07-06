#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "main.h"
#include "util.h"
#include "cache.h"

static struct {
	char *key;
	unsigned age;
} entries[36] = {0};

static char *makepath(unsigned index);

char *
makepath(unsigned index)
{
	static char buf[4096];
	snprintf(buf, sizeof buf, "%s/cache/%u", envsession, index);
	return buf;
}

why_t
cache_add(char *key, char *src)
{
	static unsigned age=0;
	why_t why;
	unsigned i, min=-1, oldest=0;
	char *dst;

	/* Find empty spot or oldest entry */
	for (i=0; i<SIZE(entries); i++) {
		if (!entries[i].key)
			break;			/* Found empty */

		if (!strcmp(entries[i].key, key)) {
			entries[i].age = age++;
			return 0;		/* Already cached */
		}

		if (entries[i].age < min) {
			min = entries[i].age;
			oldest = i;
		}
	}

	/* No empty spots, use oldest */
	if (i == SIZE(entries)) {
		i = oldest;
		free(entries[i].key);
	}

	entries[i].key = strdup(key);
	entries[i].age = age++;

	dst = makepath(i);

	if ((why = cp(src, dst)))
		return why;

	return 0;
}

char *
cache_get(char *key)
{
	unsigned i;

	for (i=0; i<SIZE(entries); i++)
		if (entries[i].key && !strcmp(entries[i].key, key))
			return makepath(i);

	return 0;
}

void
cache_cleanup()
{
	unsigned i;

	for (i=0; i<SIZE(entries); i++)
		if (entries[i].key)
			unlink(makepath(i));

	memset(entries, 0, sizeof entries);
}
