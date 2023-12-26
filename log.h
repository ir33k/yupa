/* Logger v1.6 by irek@gabr.pl                                  (-_- )
 *
 * Print formatted logger messages to target file with optional log
 * level label, trace path to source file line and exit program if
 * needed.
 *
 * This is a single header library.  When included then acts like
 * regular header file.  To compile function definitions you have to
 * predefine LOG_IMPLEMENTATION in on of your source files.
 *
 *	// main.c
 *	#define LOG_IMPLEMENTATION
 *	#include "log.h"
 *
 *	// other_file.c
 *	#include "log.h"
 *
 * Each log macro take FMT formatted message (printf(3)) as first
 * argument and optional varying number of arguments for FMT string.
 * If FMT string ends with ':' character then value of perror(0) will
 * be appended.  New line is always appended.
 *
 * Predefine LOG_LEVEL with number (0 by default) to disable logs with
 * smaller level.  You can't disable ERR and DIE as those exit program
 * and by that contribute to logic flow.  Disabling them could lead to
 * executing code that should be unreachable.  Also DEV log macro has
 * negative level to be disabled by default.
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
 *
 * Usage:
 *
 *	#define LOG_IMPLEMENTATION      // Define in one source file
 *	#include "log.h"
 *
 *	DEV("For development");
 *	LOG("Simple message");
 *	INFO("Print formatted message: %s %d", name, age);
 *	WARN("append perror, fopen %s:", file_path);
 *	ERR("print error and die");
 *	DIE("just die");
 *
 *	// main.c:9	DEV	For development
 *	// Simple message
 *	// INFO	Print formatted message: Adam 30
 *	// main.c:12	WARN	append perror, fopen file.txt: can't open
 *	// main.c:13	ERR	print error and die
 *	// just die
 *
 * Log level:
 *
 *	#define LOG_IMPLEMENTATION
 *	// Ignore DEV, LOG and INFO logs but keep WARN.
 *	#define LOG_LEVEL 2
 *	#include "log.h"
 */
#ifndef _LOG_H
#define _LOG_H

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

#define _LOG(...) _log(__FILE__, __LINE__, __VA_ARGS__)

/* Print FMT formatted string with ... varying number of arguments to
 * FP file.  Prepend string with FILENAME and LINE number if TRACE is
 * true.  Prepend LABEL if not NULL.  Kill program with DIE exit code
 * if non 0. */
static void
_log(const char *filename, int line, int trace,
     int die, FILE *fp, const char *label,
     const char *fmt, ...);

#endif /* _LOG_H */

#ifdef LOG_IMPLEMENTATION
static void
_log(const char *filename, int line, int trace,
     int die, FILE *fp, const char *label,
     const char *fmt, ...)
{
	va_list ap;
	assert(fp);
	assert(fmt);
	if (trace) {
		fprintf(fp, "%s:%d:\t", filename, line);
	}
	if (label) {
		fputs(label, fp);
		fputc('\t', fp);
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
