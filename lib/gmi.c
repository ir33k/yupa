#include <assert.h>
#include <stdio.h>
#include "le.h"
#include "gmi.h"

FILE *
gmi_req(FILE *raw, FILE *fmt, char *uri)
{
	assert(raw);
	assert(fmt);
	assert(uri);
	return 0;
}

char *
gmi_uri(FILE *body, int index)
{
	assert(body);
	assert(index > 0);
	WARN("Implement");
	return 0;
}
