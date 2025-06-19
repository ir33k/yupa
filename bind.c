#define _XOPEN_SOURCE 500	/* For strdup */
#include <stdlib.h>
#include <string.h>
#include "bind.h"

static char *binds[('Z'-'A')+1]={0};

char *
bind(char bind, char *str)
{
	int i;

	if (bind < 'A' || bind > 'Z')
		return 0;

	i = bind - 'A';

	if (str) {
		if (binds[i])
			free(binds[i]);

		binds[i] = strdup(str);
	}

	return binds[i];
}
