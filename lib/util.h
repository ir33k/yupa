// Utils.

#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define CLAMP(min,v,max) MAX(min, MIN(v, max))

// Return pointer to static string being path to user home dir like:
// "/home/user_name".  Return empty string if not defined.
char *home(void);

// Return a pointer to static string with BUFSIZ max size created by
// concatenating varying number of string args.
#define JOIN(...) _join(0, __VA_ARGS__, 0)
char *_join(int _ignore, ...);

// Return pointer to static string with random alphanumeric characters
// of LEN length.
char *strrand(size_t len);

// Create temporary file in /tmp dir with PREFIX file name prefix.
// DST is a buffer where string path to create file will be stored.
void tmpf(char *prefix, char dst[FILENAME_MAX]);

#endif // _UTIL_H
