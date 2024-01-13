// File System Protocol file://

#ifndef _FSP_H
#define _FSP_H

#include "net.h"

// Request content of URI.  Write unmodified response body to RAW open
// file.  If NET_RMT was returned then formatted response was written
// to FMT file.  If NET_URI was returned then NEW buffer will contain
// URL that should be visited new (redirection or query).
enum net_res fsp_req(FILE *raw, FILE *fmt, char *uri);

// Search in BODY open file for the link under INDEX (1 == first
// link).  Return pointer to static string with normalized URI.
char *fsp_uri(char *uri, FILE *body, int index);

#endif //_FSP_H
