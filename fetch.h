/* Send MSG to HOST:PORT with optional SSL, write response to OUT.
 * Return string on error. */
char *fetch(char *host, int port, int ssl, char *msg, FILE *out);
