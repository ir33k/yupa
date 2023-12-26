/* Logger v1.0 by irek@gabr.pl                                  (-_- )
 *
 * Print formatted logger messages to target file with optional log
 * level label, trace path to source file line and exit program if
 * needed.
 *
 * Each log macro take FMT formatted message (printf(3)) as first
 * argument and optional varying number of arguments for FMT string.
 * If FMT string ends with ':' character then value of perror(0) will
 * be appended.  New line is always appended.
 *
 * Predefine LOG_LEVEL value to disable logs with smaller level.  Note
 * that you can't disable ERR and DIE as those logs kills program and
 * by that contribute to program flow.  Ignoring them could lead to
 * executing code that should be unreachable.
 *
 * Properties:
 *
 *	macro   level   file    trace   exit    label
 *	------  ------  ------  ------  ------  ------
 *	LOG     0       stdout  no        
 *	INFO    1       stderr  no              INFO
 *	WARN    2       stderr  yes             WARN
 *	ERR             stderr  yes     1       ERR
 *	DIE             stderr  no      2
 *
 * Examples:
 *
 *	LOG("Simple message");
 *	INFO("Print formatted message: %s %d", name, age);
 *	WARN("append perror, fopen %s:", file_path);
 *	ERR("print error and die");
 *	DIE("just die");
 *
 *	// Simple message
 *	// INFO	Print formatted message: Adam 30
 *	// main.c:12	WARN	append perror, fopen file.txt: can't open
 *	// main.c:13	ERR	print error and die
 *	// just die
 *
 * Log level:
 *
 *	// Ignore LOG and INFO logs but keep WARN.
 *	#define LOG_LEVEL 2
 *	#include "log.h"
 */
#ifndef _LOG_H
#define _LOG_H

#define LOG(...)  _LOG(0, 0, stdout, 0,      __VA_ARGS__)
#define INFO(...) _LOG(0, 0, stderr, "INFO", __VA_ARGS__)
#define WARN(...) _LOG(1, 0, stderr, "WARN", __VA_ARGS__)
#define ERR(...)  _LOG(1, 1, stderr, "ERR",  __VA_ARGS__)
#define DIE(...)  _LOG(0, 2, stderr, 0,      __VA_ARGS__)

#ifndef LOG_LEVEL
#define LOG_LEVEL 0
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

/* Wrap _log() by providing FILENAME and LINE values. */
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
