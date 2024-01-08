#include <assert.h>
#include <stdio.h>
#include "le.h"
#include "gmi.h"

int
gmi_req(FILE *raw, FILE *fmt, char *host, int port, char *path)
{
	assert(raw);
	assert(fmt);
	assert(host);
	(void)port;
	(void)path;
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
