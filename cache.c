#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "main.h"
#include "cache.h"

#define CAPACITY 36

static char *keys[CAPACITY]={0};
static int keysage[CAPACITY]={0};

static char *makepath(int index);

char *
makepath(int index)
{
	static char buf[4096];
	snprintf(buf, sizeof buf, "%s/cache/%d", envtmp, index);
	return buf;
}

void
cache_add(char *key, char *src)
{
	static int age=0;
	int i, min=INT_MAX, oldest=0;
	char *dst, buf[4096];

	/* Find empty spot or oldest entry */
	for (i=0; i<CAPACITY; i++) {
		if (!keys[i])
			break;

		if (!strcmp(keys[i], key)) {
			keysage[i] = age++;
			return;			/* Already cached */
		}

		if (keysage[i] < min) {
			min = keysage[i];
			oldest = i;
		}
	}

	/* No empty spots, use oldest */
	if (i == CAPACITY) {
		i = oldest;
		free(keys[i]);
	}

	keys[i] = strdup(key);
	keysage[i] = age++;

	dst = makepath(i);

	/* TODO(irek): Replace this copy with util function. */
	snprintf(buf, sizeof buf, "cp %s %s", src, dst);
}

char *
cache_get(char *key)
{
	int i;

	for (i=0; i<CAPACITY; i++)
		if (!strcmp(keys[i], key))
			return makepath(i);

	return 0;
}

