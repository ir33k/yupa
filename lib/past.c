#include "past.h"

void
past_push(struct past *past, char *uri)
{
	assert(past);
	assert(uri);
	if (past->uri[past->i % PAST_SIZ][0]) {
		past->i++;
	}
	strncpy(past->uri[past->i % PAST_SIZ], uri, URI_SIZ);
	// Make next history item empty to cut off old forward history
	// every time the new item is being added.
	past->uri[(past->i + 1) % PAST_SIZ][0] = 0;
}

char *
past_pos(struct past *past, int offset)
{
	assert(past);
	if (offset == 0) {
		return past->uri[past->i % PAST_SIZ];
	}
	// TODO(irek): I expect that there is a bug when you loop over
	// the HSIZ and then try to get back.  I'm not checking for
	// the empty history entry.  Also when going forward I'm
	// looking only at very next entry but value of OFFSET can be
	// more then 1.
	if (offset > 0 && !past->uri[(past->i + 1) % PAST_SIZ][0]) {
		return 0;
	}
	if (offset < 0 && !past->i) {
		return 0;
	}
	past->i += offset;
	return past->uri[past->i % PAST_SIZ];
}
