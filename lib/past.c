#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "le.h"
#include "past.h"

#define URI_INDEX(past) ((past->i % past->n) * past->siz)
#define URI_OFFSET(past, offset) (((past->i+(offset)) % past->n) * past->siz)

struct past *
past_new(size_t n, size_t siz)
{
	struct past *past;
	size_t uri_siz = sizeof(char) * n * siz;
	if (!(past = malloc(sizeof(*past)))) {
		ERR("malloc:");
	}
	memset(past, 0, sizeof(*past));
	past->n = n;
	past->siz = siz;
	if (!(past->uri = malloc(uri_siz))) {
		ERR("malloc:");
	}
	memset(past->uri, 0, uri_siz);
	return past;
}

void
past_set(struct past *past, char *uri)
{
	assert(past);
	if (!uri || !uri[0]) {
		return;
	}
	if (past->uri[URI_INDEX(past)]) {
		past->i++;
	}
	strncpy(past->uri + URI_INDEX(past), uri, past->siz);
	// Make next history item empty to cut off old forward history
	// every time the new item is being added.
	past->uri[URI_OFFSET(past, 1)] = 0;
}

char *
past_get(struct past *past, int offset)
{
	assert(past);
	if (!offset) {
		return past->uri + URI_INDEX(past);
	}
	if (offset < 0 && past->i < (size_t)(offset*-1)) {
		return 0;
	}
	// TOOD(irek): Handle offset by more than 1?
	if (!past->uri[URI_OFFSET(past, offset > 0 ? 1 : -1)]) {
		return 0;
	}
	past->i += offset;
	return past->uri + URI_INDEX(past);
}

void
past_free(struct past *past)
{
	free(past->uri);
	free(past);
}
