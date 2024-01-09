// Everything socket, tcp, internet related.

#ifndef _NET_H
#define _NET_H

// Establish AF_INET internet SOCK_STREAM stream connection to HOST of
// PORT.  Return socket file descriptor or 0 on error.
int tcp(char *host, int port);

//
// TODO(irek): I dislike this function.  Merging it with onuri?
//
// Open connection to server under HOST with PORT and optional PATH.
// Return socket file descriptor on success that is ready to read
// server response.  Return 0 on error.
int req(char *host, int port, char *path);

#endif // _NET_H
