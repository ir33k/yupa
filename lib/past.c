#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "le.h"
#include "past.h"

// Return index to PAST->URI ring buffer based on N max number of
// items and SIZ items size.
#define URI_AT(past, i) (((i) % (past)->n) * (past)->sz)

// Like URI_AT but for currently active item.
#define URI_I(past) URI_AT((past), (past)->i)

// Like URI_AT but for currently active item moved by OFFSET.
#define URI_OF(past, offset) URI_AT((past), (past)->i+(offset))

struct past *
past_new(size_t n, size_t sz)
{
	struct past *past;
	size_t uri_sz = sizeof(char) * n * sz;
	assert(n > 0);
	assert(sz > 0);
	if (!(past = malloc(sizeof(*past)))) {
		ERR("malloc:");
	}
	memset(past, 0, sizeof(*past));
	past->n = n;
	past->sz = sz;
	if (!(past->uri = malloc(uri_sz))) {
		ERR("malloc:");
	}
	memset(past->uri, 0, uri_sz);
	return past;
}

void
past_set(struct past *past, char *uri)
{
	assert(past);
	if (!uri || !uri[0]) {
		return;
	}
	if (past->uri[URI_I(past)]) {
		past->i++;
	}
	strncpy(past->uri + URI_I(past), uri, past->sz);
	// Make next history item empty to cut off old forward history
	// every time the new item is being added.
	past->uri[URI_OF(past, 1)] = 0;
}

char *
past_get(struct past *past, int offset)
{
	assert(past);
	// TODO(irek): Handle offset by more than 1?
	assert(offset == 0 || offset == -1 || offset == 1);
	if (!offset) {
		return past->uri + URI_I(past);
	}
	if (offset < 0 && past->i < (size_t)(offset*-1)) {
		return 0;
	}
	if (!past->uri[URI_OF(past, offset > 0 ? 1 : -1)]) {
		return 0;
	}
	past->i += offset;
	return past->uri + URI_I(past);
}

void
past_print(struct past *past)
{
	size_t i, j;
	assert(past);
	i = past->i;
	while (i && past->uri[URI_AT(past, i)]) i--;
	if (!past->uri[URI_AT(past, i)]) {
		i++;
	}
	for (j=0 ; past->uri[URI_AT(past, i)]; i++, j++) {
		printf("\t%lu: %s%s\n", j,
		       i == past->i ? "> " : "  ",
		       past->uri + URI_AT(past, i));
	}
}

void
past_free(struct past *past)
{
	assert(past);
	free(past->uri);
	free(past);
}
