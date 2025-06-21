#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void
cache_add(char *key, char *src)
{
	static unsigned age=0;
	unsigned i, min=-1, oldest=0;
	char *dst, buf[4096];

	/* Find empty spot or oldest entry */
	for (i=0; i<SIZE(entries); i++) {
		if (!entries[i].key)
			break;

		if (!strcmp(entries[i].key, key)) {
			entries[i].age = age++;
			return;			/* Already cached */
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

	/* TODO(irek): Replace this copy with util function. */
	snprintf(buf, sizeof buf, "cp %s %s", src, dst);
}

char *
cache_get(char *key)
{
	unsigned i;

	for (i=0; i<SIZE(entries); i++)
		if (!strcmp(entries[i].key, key))
			return makepath(i);

	return 0;
}
