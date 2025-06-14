#include "util.h"

char *
online(char **str)
{
	char *line;

	if (!**str)
		return 0;

	line = *str;

	while (**str && **str != '\n' && **str != '\r') (*str)++;
	if (**str) {
		**str = 0;
		(*str)++;

		if (**str == '\n')	/* Handle \r\n case */
			(*str)++;
	}

	return line;
}
