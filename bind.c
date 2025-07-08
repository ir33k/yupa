#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "util.h"
#include "bind.h"

static char *binds['Z'-'A'+1]={0};
static void save(char *path);

void
save(char *path)
{
	int i;
	FILE *fp;

	if (!(fp = fopen(path, "w")))
		err(1, "bind save fopen %s", path);

	for (i=0; i<COUNT(binds); i++)
		if (binds[i])
			fprintf(fp, "%c\t%s\n", i+'A', binds[i]);

	if (fclose(fp))
		err(1, "bind save fclose %s", path);
}

void
bind_init(char *path)
{
	char buf[4096];
	FILE *fp;

	if (!(fp = fopen(path, "r")))
		return;		/* Ignore error, file might not exist */

	while (fgets(buf, sizeof buf, fp))
		binds[buf[0]-'A'] = strdup(trim(buf+1));

	if (fclose(fp))
		err(1, "bind_init flose %s", path);
}

void
bind_set(char bind, char *str, char *path)
{
	int i = bind-'A';

	if (binds[i])
		free(binds[i]);

	binds[i] = strdup(str);
	save(path);
}

char *
bind_get(char bind)
{
	return binds[bind-'A'];
}
