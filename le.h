/* le.h (LogErr.Header) the logger v1.0

The LOG, WARN and ERR macros prints current file name, line number,
function name, log prefix and FMT formatted message (printf(3)) with
optional varying number of arguments for FMT.  When FMT ends with ':'
then result of perror(0) is appended.  When ERR macro is used then
program exits with 1.

	$ cat -n demo.c
	     1	#include "le.h"
	     2	int main(void)
	     3	{
	     4		LOG("Print args %d %s", 1, "test");
	     5		WARN("With perror, fopen:");
	     6		ERR("Print error and exit with 1");
	     7		return 0;
	     8	}

	$ cc demo.c && ./a.out
	demo.c:4	main()	>>>> Print args 1 test
	demo.c:5	main()	WARN With perror, fopen: can't open
	demo.c:6	main()	ERR Print error and exit with 1

ISC-License

Copyright 2023 Irek <irek@gabr.pl>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG(...)  _LEH_LOG(">>>> " __VA_ARGS__, 0)
#define WARN(...) _LEH_LOG("WARN " __VA_ARGS__, 0)
#define ERR(...)  _LEH_LOG("ERR "  __VA_ARGS__, 1)
#define _LEH_LOG(fmt, ...) _leh_log("%s:%d:\t%s()\t" fmt, \
	__FILE__, __LINE__, __func__, __VA_ARGS__)

static void
_leh_log(const char *fmt, ...)
{
	va_list ap;
	int die;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	die = va_arg(ap, int);
	va_end(ap);
	if (fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(0);
	} else {
		fputc('\n', stderr);
	}
	if (die) {
		exit(die);
	}
}
