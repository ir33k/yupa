/* Extract parts of URI string */

/* Protocols defined as default ports */
enum { HTTP=80, HTTPS=443, GEMINI=1965, GOPHER=70 };

int uri_protocol(char *);
char *uri_host(char *);
int uri_port(char *);
char *uri_path(char *);
