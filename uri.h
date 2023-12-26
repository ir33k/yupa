#ifndef _URI_H
#define _URI_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define URI_SIZ	1024            /* Max size of URI */

enum uri_protocol {
	URI_PROTOCOL_NONE = -1, /* Protocol defined but none of those below */
	URI_PROTOCOL_NUL  =  0, /* For empty, not defined protocol */
	URI_PROTOCOL_GOPHER,
	URI_PROTOCOL_GEMINI,
	URI_PROTOCOL_HTTPS,
	URI_PROTOCOL_HTTP,
	URI_PROTOCOL_FILE,
	/* I will not support those but it's nice to recognize them. */
	URI_PROTOCOL_SSH,
	URI_PROTOCOL_FTP
};

/* For given PROTOCOL enum return string representation. */
char *uri_protocol_str(enum uri_protocol protocol);

/* Get protocol from URI string. */
enum uri_protocol uri_protocol(char *uri);

/* Get host from URI string.  Return NULL if not found.  Return
 * pointer to static string on success. */
char *uri_host(char *uri);

/* Get port from URI string.  Return 0 by default. */
int uri_port(char *uri);

/* Return pointer to first URI string path character.  Return NULL if
 * not found. */
char *uri_path(char *uri);

/* Construct normalized URI in DST buffer from given URI parts, where
 * only PATH can be NULL. */
void uri_normalize(char dst[URI_SIZ], enum uri_protocol protocol,
			  char *host, int port, char *path);

#endif /* _URI_H */
