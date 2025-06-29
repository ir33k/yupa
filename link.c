#include <err.h>
#include <stdlib.h>
#include <string.h>
#include "link.h"

static char **links=0;
unsigned count=0, capacity=0;

void
link_clear()
{
	while (count)
		free(links[--count]);
}

unsigned
link_store(char *uri)
{
	if (count+1 >= capacity) {
		capacity = capacity ? capacity*2 : 32;
		links = realloc(links, sizeof(char *) * capacity);
		if (!links)
			err(1, "link_store realloc %s %u", uri, capacity);
	}
	links[count++] = strdup(uri);
	return count -1;
}

char *
link_get(unsigned i)
{
	return i >= count ? 0 : links[i];
}
