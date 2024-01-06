// Past browsing history ring buffer.

#ifndef _PAST_H
#define _PAST_H

#include "uri.h"

#ifndef PAST_SIZ
#define PAST_SIZ 64
#endif

struct past {
	char uri[PAST_SIZ][URI_SIZ];
	size_t i;
};

//
void past_push(struct past *past, char *uri);

//
char *past_pos(struct past *past, int offset);

#endif // _PAST_H
