// Gemini protocol.

#ifndef _GMI_H
#define _GMI_H

#include <stdio.h>

// Search in BODY open file for the link under INDEX (1 == first
// link).  Return pointer to static string with normalized URI.
char *gmi_uri(FILE *body, int index);

#endif //_GMI_H
