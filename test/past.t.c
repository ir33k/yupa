#include "walter.h"
#include "../lib/le.h"
#include "../lib/past.h"

// Useful function for debugging printing content of whole PAST
// internal buffer marking ucrrently indexed element.  Not used
// outside of debugging sessins.
#pragma GCC diagnostic ignored "-Wunused-function"
static void
past_dump(struct past *past)
{
	size_t i;
	fputc('\n', stdout);
	for (i=0; i < past->n; i++) {
		printf("%lu: %s%s\n", i,
		       i == past->i ? "> " : "  ",
		       past->uri + i*past->siz);
	}
}
#pragma GCC diagnostic pop

TEST("past_set and past_get")
{
	struct past *past;

	OK(past = past_new(4, 32));
	OK(past->uri[0] == 0);
	OK(past->i == 0);

	past_set(past, "aaa");
	past_set(past, "bbb");
	past_set(past, "ccc");
	SEQ(past_get(past, 0), "ccc");
	SEQ(past_get(past, 0), "ccc");
	OK(past->i == 2);
	SEQ(past_get(past, -1), "bbb");
	SEQ(past_get(past, 0), "bbb");
	SEQ(past_get(past, -1), "aaa");
	SEQ(past_get(past, 0), "aaa");
	OK(past_get(past, -1) == 0);
	OK(past_get(past, -1) == 0);
	SEQ(past_get(past, 0), "aaa");
	SEQ(past_get(past, 1), "bbb");
	SEQ(past_get(past, 0), "bbb");
	SEQ(past_get(past, 1), "ccc");
	OK(past_get(past, 1) == 0);
	past_set(past, "ddd");
	past_set(past, "eee");
	OK(past_get(past, 1) == 0);
	SEQ(past_get(past, 0), "eee");
	SEQ(past_get(past, -1), "ddd");
	SEQ(past_get(past, -1), "ccc");
	OK(past_get(past, -1) == 0);
	OK(past_get(past, -1) == 0);
	SEQ(past_get(past, 0), "ccc");

	past_free(past);
}
