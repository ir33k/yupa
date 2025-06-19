/* Extract parts of URI string */

enum { LOCAL=1, HTTP=80, HTTPS=443, GEMINI=1965, GOPHER=70 };

#define URI_DEFAULT_PROTOCOL LOCAL

int uri_protocol(char *uri);
char *uri_host(char *uri);
int uri_port(char *uri);
char *uri_path(char *uri);
char *uri_normalize(char *link, char *base_uri);
