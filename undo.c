#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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
	static const int LIMIT = 10*1024;
	static char date[32], *buf=0;
	char *mode;
	time_t timestamp;
	int n;
	struct tm *tm;
	FILE *fp;

	if (startswith(uri_path(uri), envhome))
		return;

	if (!buf && !(buf = malloc(LIMIT)))
		err(1, "undo save malloc");

	mode = access(path, F_OK) ? "w+" : "r+";
	if (!(fp = fopen(path, mode)))
		err(1, "undo save fopen %s", path);

	if (fseek(fp, 0, SEEK_SET) == -1)
		err(1, "undo save fseek %s", path);

	if ((timestamp = time(0)) == (time_t) -1)
		err(1, "undo save time");

	if (!(tm = localtime(&timestamp)))
		err(1, "undo save localtime");

	strftime(date, sizeof date, "%Y.%m.%d", tm);
	n = snprintf(buf, LIMIT, "=> %s\t%s %s\n", uri, date, uri);
	n += fread(buf+n, 1, LIMIT-n, fp);

	if (fseek(fp, 0, SEEK_SET) == -1)
		err(1, "undo save fseek %s", path);

	fwrite(buf, 1, n, fp);

	if (fclose(fp))
		err(1, "undo save fclose %s", path);
}

void
undo_load(char *path)
{
	(void)path;
	// TODO(irek): Load last entires from history.gmi file.
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
