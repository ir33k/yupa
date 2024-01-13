// Protocols gemini://

#ifndef _GMI_H
#define _GMI_H

#include "net.h"

// Request content of URI.  Write unmodified response body to RAW open
// file.  If NET_RMT was returned then formatted response was written
// to FMT file.  If NET_URI was returned then NEW buffer will contain
// URL that should be visited new (redirection or query).
enum net_res gmi_req(FILE *raw, FILE *fmt, char *uri, char *new);

// Search in BODY open file for the link under INDEX (1 == first
// link).  Return pointer to static string with normalized URI.
char *gmi_uri(FILE *body, int index);

//
void gmi_fmt(FILE *src, FILE *dst);

#endif //_GMI_H
