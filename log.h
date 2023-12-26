/* Logger v1.9 by irek@gabr.pl updated 2023.12.26 Tue 13:16 UTC
 *
 * Print formatted log messages like INFO, WARNING, ERROR to stdout or
 * stderr with trace path to source file, line number, function name
 * and optional standard errno string then exit program if needed.
 *
 * TODO(irek): Think of unique name: Galgo, Carter, Papkin?  Someone
 * who talks a lot or writes a lot?
 *
 * Table of contents:
 *
 *	Single header
 *	Usage
 *	Log level
 *	Properties
 *	MIT License (at the very end of this file)
 *
 * Single header:
 *
 *	This is a single header library.  When included acts like
 *	regular header file.  To compile function definitions you have
 *	to predefine LOG_IMPLEMENTATION in one of your source files.
 *
 *	// main.c
 *	#define LOG_IMPLEMENTATION
 *	#include "log.h"
 *
 *	// other_file.c
 *	#include "log.h"
 *
 * Usage:
 *
 *	Each log macro take FMT formatted message (printf(3)) as first
 *	argument and optional varying number of arguments for FMT.  If
 *	FMT string ends with ':' char then value of perror(0) will be
 *	appended.  New line is always appended.
 *
 *	$ cat -n demo.c
 *	     1	#define LOG_IMPLEMENTATION      // Define in one source file
 *	     2	#define LOG_LEVEL -1            // Enable DEV logs
 *	     3	#include "log.h"
 *	     4
 *	     5	int main(void)
 *	     6	{
 *	     7		DEV("For debugging %s %d", argv[0], bool);
 *	     8		LOG("Simple log message");
 *	     9		INFO("Print formatted message: %s %d", name, age);
 *	    10		WARN("With perror, fopen:");
 *	    11		ERR("Print error and die");     // Kill program
 *	    12		DIE("Just die");                // Kill program
 *	    13		return 0;
 *	    14	}
 *
 *	$ cc demo.c && ./a.out
 *	demo.c:7	>>>>	main() For debugging ./main 0
 *	Simple log message
 *	INFO	Print formatted message: Adam 30
 *	demo.c:10	WARN	main() With perror, fopen: can't open
 *	demo.c:11	ERR	main() Print error and die
 *	Just die
 *
 * Log level:
 *
 *	Predefine LOG_LEVEL with number (0 by default) to disable logs
 *	with smaller level.  You can't disable ERR and DIE as those
 *	exit program and by that contribute to logic flow.  Disabling
 *	them could lead to executing code that should be unreachable.
 *	DEV log macro has negative level to be disabled by default.
 *
 *	// Ignore DEV, LOG and INFO logs but keep WARN.
 *	#define LOG_LEVEL 2
 *	#include "log.h"
 *
 *	// Enable all logs with DEV that is disabled by default.
 *	#define LOG_LEVEL -1
 *	#include "log.h"
 *
 * Properties:
 *
 *	macro   level   file    trace   exit    label
 *	------  ------  ------  ------  ------  ------
 *	DEV     -1      stderr  yes             >>>>
 *	LOG      0      stdout  no
 *	INFO     1      stderr  no              INFO
 *	WARN     2      stderr  yes             WARN
 *	ERR             stderr  yes     1       ERR
 *	DIE             stderr  no      2
 */
#ifndef _LOG_H
#define _LOG_H

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define DEV(...)  _LOG(1, 0, stderr, ">>>>", __VA_ARGS__)
#define LOG(...)  _LOG(0, 0, stdout, 0,      __VA_ARGS__)
#define INFO(...) _LOG(0, 0, stderr, "INFO", __VA_ARGS__)
#define WARN(...) _LOG(1, 0, stderr, "WARN", __VA_ARGS__)
#define ERR(...)  _LOG(1, 1, stderr, "ERR",  __VA_ARGS__)
#define DIE(...)  _LOG(0, 2, stderr, 0,      __VA_ARGS__)

#ifndef LOG_LEVEL
#define LOG_LEVEL 0
#endif

#if LOG_LEVEL > -1
#undef DEV
#define DEV(...)
#endif

#if LOG_LEVEL > 0
#undef LOG
#define LOG(...)
#endif

#if LOG_LEVEL > 1
#undef INFO
#define INFO(...)
#endif

#if LOG_LEVEL > 2
#undef WARN
#define WARN(...)
#endif

/* From https://gcc.gnu.org/onlinedocs/gcc-4.8.1/gcc/Function-Names.html */
#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#define _LOG(...) _log(__FILE__, __LINE__, __func__, __VA_ARGS__)

/* Print FMT formatted string with ... varying number of arguments to
 * FP file.  Prepend string with FILENAME, LINE number and FUNCTION
 * name if TRACE is true.  Prepend LABEL if not NULL.  Kill program
 * with DIE exit code if non 0. */
static void
_log(const char *filename, int line, const char *function,
     int trace, int die, FILE *fp, const char *label, const char *fmt, ...);

#endif /* _LOG_H */
#ifdef LOG_IMPLEMENTATION

static void
_log(const char *filename, int line, const char *function,
     int trace, int die, FILE *fp, const char *label, const char *fmt, ...)
{
	va_list ap;
	assert(fmt);
	if (trace) {
		fprintf(fp, "%s:%d:\t", filename, line);
	}
	if (label) {
		fputs(label, fp);
		fputc('\t', fp);
	}
	if (trace) {
		fprintf(fp, "%s() ", function);
	}
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', fp);
		perror(0);
	} else {
		fputc('\n', fp);
	}
	if (die) {
		exit(die);
	}
}
#endif /* LOG_IMPLEMENTATION */
/*
MIT License (https://mit-license.org/), Copyright (c) 2023 irek

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
