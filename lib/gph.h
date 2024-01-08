// Gopher protocol.

#ifndef _GPH_H
#define _GPH_H

#include <stdio.h>

//
int gph_req(FILE *raw, FILE *fmt, char *host, int port, char *path);

// Assuming that BODY is an open file with Gopher submenu, write
// prettier formatted version to open DST file.
void gph_fmt(FILE *body, FILE *dst);

// Search in BODY open file for the link under INDEX (1 == first
// link).  Return pointer to static string with normalized URI.
char *gph_uri(FILE *body, int index);

#endif // _GPH_H
