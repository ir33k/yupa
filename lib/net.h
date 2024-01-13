// Common network code for protocols: gph.h, gmi.h, ...

#ifndef _NET_H
#define _NET_H

#include <stdio.h>

enum net_res {          // Protocol request response type
	NET_NUL = 0,    // Empty result, nothing has happen
	NET_ERR,        // Request failed, URL invalid or host broken
	NET_RAW,        // Show RAW body text response
	NET_FMT,        // Show FMT formatted text response
	NET_BIN,        // Request resulted in non printable binary data
	NET_URI,        // Request resulted in new URI, redirect or query
};

// Establish AF_INET internet SOCK_STREAM stream connection to HOST of
// PORT.  Return socket file descriptor or 0 on error.
int net_tcp(char *host, int port);

//
// TODO(irek): I dislike this function.
//
// Open connection to server under HOST with PORT and optional PATH.
// Return socket file descriptor on success that is ready to read
// server response.  Return 0 on error.
int net_req(char *host, int port, char *path);

#endif // _NET_H
