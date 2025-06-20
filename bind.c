#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "bind.h"

static char *binds[('Z'-'A')+1]={0};
static char lastpath[4096]={0};

static void save();

void
save()
{
	char c, *str;
	FILE *fp;

	if (!lastpath[0])
		return;

	fp = fopen(lastpath, "w");
	if (!fp)
		err(1, "bind save fopen");

	for (c = 'A'; c <= 'Z'; c++) {
		str = bind_get(c);

		if (str)
			fprintf(fp, "%c\t%s\n", c, str);
	}

	if (fclose(fp))
		err(1, "bind save fclose");
}


void
bind_set(char bind, char *str)
{
	int i;

	if (bind < 'A' || bind > 'Z')
		return;

	i = bind - 'A';

	if (binds[i]) {
		free(binds[i]);
		binds[i] = 0;
	}

	if (str) {
		trimr(str);
		str = triml(str);

		if (str[0])
			binds[i] = strdup(triml(str));
	}

	save();
}

char *
bind_get(char bind)
{
	if (bind < 'A' || bind > 'Z')
		return 0;

	return binds[bind - 'A'];
}

void
bind_load(char *path)
{
	char buf[4096];
	FILE *fp;

	strcpy(lastpath, path);

	fp = fopen(path, "r");

	/* NOTE(irek): Ignore error on purpose.  The most propable
	 * error here is that it was not possible to load/find binds
	 * file.  This is not a critical error as fresh installation
	 * of Yupa expect no binds.  Later binds file will be created
	 * when first bind is defined. */
	if (!fp)
		return;

	while (fgets(buf, sizeof buf, fp))
		bind_set(buf[0], buf+2);

	if (fclose(fp))
		err(1, "bind_load flose");
}
