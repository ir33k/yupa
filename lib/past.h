// Past browsing history ring buffer.

#ifndef _PAST_H
#define _PAST_H

#include <stdio.h>

struct past {                   // History ring buffer
	// Readonly
	char   *uri;            // Array of URIs
	size_t  n, siz;         // Array items count and their size
	size_t  i;              // Index to current URI
};

//
struct past *past_new(size_t n, size_t siz);

// Add new URI to PAST history buffer at current index position.
// Entries beyond new entry are no longer available after that.
void past_set(struct past *past, char *uri);

// Move in PAST by OFFSET from current index position returning URI
// under new index.
char *past_get(struct past *past, int offset);

//
void past_free(struct past *past);

#endif // _PAST_H