/* Logger v1.0 by irek@gabr.pl

TODO(irek): Give this lib an unique name.

Print formatted log messages like LOG, WARNING, ERROR to stdout or
stderr with trace path to source file, line number, function name and
optional standard errno string then exit program if needed.

Table of contents:

	Single header
	Usage
	Log file
	Log level
	Properties
	Change log
	MIT License (at the very end of this file)

Single header:

	This is a single header library.  When included acts like
	regular header file.  To compile function definitions you have
	to predefined LOG_IMPLEMENTATION in one of your source files.

	$ cat -n main.c
	     1	#define LOG_IMPLEMENTATION
	     2	#include "log.h"
	     3	// ...

	$ cat -n other.c
	     1	#include "log.h"
	     2	// ...

Usage:

	Each log macro take FMT formatted message (printf(3)) as first
	argument and optional varying number of arguments for FMT.  If
	FMT string ends with ':' char then value of perror(0) will be
	appended.  New line is always appended.

	$ cat -n demo.c
	     1	#define LOG_IMPLEMENTATION      // Define in one file
	     2	#define LOG_FILE stderr         // Logs target file
	     2	#define LOG_LEVEL -1            // Enable DEV logs
	     3	#include "log.h"
	     4
	     5	int main(void)
	     6	{
	     7		DEV("For debugging %s %d", argv[0], bool);
	     8		LOG("Simple log message");
	    10		WARN("With perror, fopen:");
	    11		ERR("Print error and exit with 1");
	    13		return 0;
	    14	}

	$ cc demo.c && ./a.out
	demo.c:7	main()	>>>> For debugging ./main 0
	demo.c:8	main()	LOG Simple log message
	demo.c:9	main()	WARN With perror, fopen: can't open
	demo.c:10	main()	ERR Print error and exit with 1

Log file:

	By default logs are being printed to stderr but you can change
	that by redefining LOG_FILE.  OFC for that to work you have to
	first open custom file and have it as global pointer.

	// Use different FILE pointer for logs (default stderr).
	#define LOG_FILE my_logs_file_pointer
	#include "log.h"

Log level:

	Predefined LOG_LEVEL with number (0 by default) to disable
	logs with smaller level.  You can't disable ERR as it will
	exit program and by that contribute to logic flow.  Disabling
	it could lead to executing code that should be unreachable.
	DEV log macro has negative level to be disabled by default.

	// Ignore DEV and LOG logs but keep WARN.
	#define LOG_LEVEL 1
	#include "log.h"

	// Enable all logs with DEV that is disabled by default.
	#define LOG_LEVEL -1
	#include "log.h"

Properties:

	macro   level   label   exit
	------  ------  ------  ------
	DEV     -1      >>>>    no
	LOG      0      LOG     no
	WARN     1      WARN    no
	ERR             ERR     yes

Change log:

	2023.12.26 Tue 18:32	v1.0 Initial motivation

	I was tired of writing printf by hand with context information
	like function name.  Usually I was killing my programs as soon
	as I encountered and error from std lib functions like fopen()
	using die() function stolen from suckless.org projects. BTW I
	love the way you append perror message by ending log message
	with ':'.  Most of the times this is great simple approach but
	in last project I found myself in need of printing log without
	killing the program.  Those reasons lead me to creating this
	logger lib.

	I just want to append context data like file name, line number
	and function name automatically when printing log message with
	some arguments and optional perror message.  For most cases I
	use ERR to kill program as soon as possible but when necessary
	I will use WARN.  While printf debugging I can use DEV to have
	context data and there is simple LOG for completion.
*/
#ifndef _LOG_H
#define _LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define DEV(...)  _LOG(">>>>", 0, __VA_ARGS__)
#define LOG(...)  _LOG("LOG",  0, __VA_ARGS__)
#define WARN(...) _LOG("WARN", 0, __VA_ARGS__)
#define ERR(...)  _LOG("ERR",  1, __VA_ARGS__)

/* From https://gcc.gnu.org/onlinedocs/gcc-4.8.1/gcc/Function-Names.html */
#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#define _LOG(...) _log(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifndef LOG_FILE
#define LOG_FILE stderr
#endif

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
#undef WARN
#define WARN(...)
#endif

/* Print FMT formatted message with varying number of args.  Prepend
 * string with FILENAME, LINE number, FUNCTION name and LABEL.  Kill
 * program with DIE exit code if non 0. */
static void
_log(const char *filename, int line, const char *function,
     const char *label, int die, const char *fmt, ...);

#endif /* _LOG_H */
#ifdef LOG_IMPLEMENTATION

static void
_log(const char *filename, int line, const char *function,
     const char *label, int die, const char *fmt, ...)
{
	va_list ap;
	fprintf(LOG_FILE, "%s:%d:\t%s()\t%s ",
		filename, line, function, label);
	va_start(ap, fmt);
	vfprintf(LOG_FILE, fmt, ap);
	va_end(ap);
	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', LOG_FILE);
		perror(0);
	} else {
		fputc('\n', LOG_FILE);
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
