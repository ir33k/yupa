#include <string.h>
#include "undo.h"

#define SIZE 32

static char ring[SIZE][4096]={0};
static unsigned last=0;

void
undo_add(char *str)
{
	if (strcmp(ring[last % SIZE], str)) {
		strcpy(ring[++last % SIZE], str);
		ring[(last+1) % SIZE][0] = 0;
	}
}

char *
undo_go(int n)
{
	int i=n, d = n>0 ? -1 : +1;

	for (; last && i; i+=d, last-=d)
		if (!ring[(last -d) % SIZE][0])
			break;

	return i == n ? 0 : ring[last % SIZE];
}
