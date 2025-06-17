/* Extract parts of URI string */

enum { HTTP=80, HTTPS=443, GEMINI=1965, GOPHER=70 };

/* TODO(irek): I'm not sure what should be the default, maybe make it
 * an environment variable? */
#define URI_DEFAULT_PROTOCOL GEMINI

int uri_protocol(char *uri);
char *uri_host(char *uri);
int uri_port(char *uri);
char *uri_path(char *uri);
char *uri_normalize(char *link, char *base_uri);
