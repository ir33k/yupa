#include "walter.h"
#include "../lib/past.h"

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

	past_free(past);
}
