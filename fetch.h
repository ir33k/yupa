/* Send MSG string to HOST:PORT with optional SSL, write response to
 * OUT file.  CRLF will be automatically added at the end of the MSG.
 * Return string on error. */
why_t fetch(char *host, int port, int ssl, char *msg, char *out);
