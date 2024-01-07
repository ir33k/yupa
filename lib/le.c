#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "le.h"

void
_leh_log(const char *fmt, ...)
{
	va_list ap;
	int die;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	die = va_arg(ap, int);
	va_end(ap);
	if (fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(0);
	} else {
		fputc('\n', stderr);
	}
	if (die) {
		exit(die);
	}
}
