#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "le.h"
#include "past.h"

#define URI_INDEX(past) ((past->i % past->n) * past->siz)
#define URI_NEXT(past) (((past->i+1) % past->n) * past->siz)

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
	past->uri[URI_NEXT(past)] = 0;
}

char *
past_get(struct past *past, int offset)
{
	assert(past);
	if (offset == 0) {
		return past->uri + URI_INDEX(past);
	}
	// TODO(irek): I expect that there is a bug when you loop over
	// the HSIZ and then try to get back.  I'm not checking for
	// the empty history entry.  Also when going forward I'm
	// looking only at very next entry but value of OFFSET can be
	// more then 1.
	if (offset > 0 && !past->uri[URI_NEXT(past)]) {
		return 0;
	}
	if (offset < 0 && !past->i) {
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
