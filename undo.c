#include <stdio.h>
#include <string.h>
#include "undo.h"

/* NOTE(irek): This browser undo history is rather small.  But browser
 * will most likely be used for short session and if there will be a
 * need to go back further in history then persistent undo history
 * file can be used instead. */

#define SIZE 64

static char ring[SIZE][4096]={0};
static unsigned last=0;

void
undo_add(char *uri)
{
	if (!strcmp(ring[last % SIZE], uri))
		return;

	last++;
	strcpy(ring[last % SIZE], uri);
	ring[(last+1) % SIZE][0] = 0;
}

char *
undo_go(int offset)
{
	if (last == 0)
		return 0;

	while (offset < 0) {
		if (ring[(last-1) % SIZE][0] == 0)
			return 0;

		last--;
		offset++;
	}

	while (offset > 0) {
		if (ring[(last+1) % SIZE][0] == 0)
			return 0;

		last++;
		offset--;
	}

	return ring[last % SIZE];
}
