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
		if (**str == '\r') {
			**str = 0;
			(*str)++;
		}
		**str = 0;
		(*str)++;
	}

	return line;
}

char *
triml(char *str)
{
	while (*str && *str <= ' ') str++;
	return str;
}
