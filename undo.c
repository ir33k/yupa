#include <err.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "main.h"
#include "util.h"
#include "uri.h"
#include "undo.h"

#define SIZE 32

static char ring[SIZE][4096]={0};
static unsigned last=0;

static void save(char *uri, char *path);

void
save(char *uri, char *path)
{
	char buf[256];
	FILE *fp;
	time_t timestamp;
	struct tm *tm;

	if (startswith(uri_path(uri), envhome))
		return;

	if (!(fp = fopen(path, "a")))
		err(1, "undo save fopen %s", path);

	if ((timestamp = time(0)) == (time_t) -1)
		err(1, "undo save time");

	if (!(tm = localtime(&timestamp)))
		err(1, "undo save localtime");

	strftime(buf, sizeof buf, "%Y.%m.%d", tm);
	fprintf(fp, "=> %s\t%s %s\n", uri, buf, uri);

	if (fclose(fp))
		err(1, "undo save flose %s", path);
}

void
undo_add(char *uri, char *path)
{
	if (!strcmp(ring[last % SIZE], uri))
		return;

	strcpy(ring[++last % SIZE], uri);
	ring[(last+1) % SIZE][0] = 0;
	save(uri, path);
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
