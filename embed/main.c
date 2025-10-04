/* Convert stdin to C string variable in stdout

Usage:

	$ embed variable_name < file.txt
	const char *variable_name =
		"File first line\n"
		"File second line\n"
		;
*/

#include <stdio.h>
#include <string.h>

int
main(int argc, char **argv)
{
	char buf[4096];
	int i;

	if (argc < 2) {
		fprintf(stderr, "usage: %s variable_name < input\n", argv[0]);
		return 1;
	}

	printf("const char *%s =\n", argv[1]);

	while (fgets(buf, sizeof buf, stdin)) {
		printf("	\"");
		for (i=0; buf[i]; i++)
			switch (buf[i]) {
			case '\\': printf("\\"); break;
			case '"': printf("\\\""); break;
			case '\n': printf("\\n"); break;
			default: putchar(buf[i]);
			}
		printf("\"\n");
	}

	printf("	;\n");

	return 0;
}
