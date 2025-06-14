#include <stdio.h>
#include "gph.h"

void
gph_print(char *in, FILE *out)
{
	fprintf(out, "GPH:\n%s\n", in);
}
