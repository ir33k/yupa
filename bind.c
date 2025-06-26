#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "util.h"
#include "bind.h"

static char *binds['Z'-'A'+1]={0};
static char *path;
static void save();

void
save()
{
	int i;
	FILE *fp;

	if (!(fp = fopen(path, "w")))
		err(1, "bind save fopen");

	for (i=0; i<SIZE(binds); i++)
		if (binds[i])
			fprintf(fp, "%c\t%s\n", i+'A', binds[i]);

	if (fclose(fp))
		err(1, "bind save fclose");
}

void
bind_init()
{
	char buf[4096];
	FILE *fp;

	path = join(envhome, "/binds");

	if (!(fp = fopen(path, "r")))
		return;		/* Ignore error, file might not exist */

	while (fgets(buf, sizeof buf, fp))
		binds[buf[0]-'A'] = strdup(trim(buf+1));

	if (fclose(fp))
		err(1, "bind_init flose");
}

void
bind_set(char bind, char *str)
{
	int i = bind-'A';

	if (binds[i])
		free(binds[i]);

	binds[i] = strdup(str);
	save();
}

char *
bind_get(char bind)
{
	return binds[bind-'A'];
}
