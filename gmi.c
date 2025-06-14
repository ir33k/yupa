#include <stdio.h>
#include "gmi.h"

void
gmi_print(char *in, FILE *out)
{
	fprintf(out, "GMI:\n%s\n", in);
}
