/* Extract parts of URI string */

/* https://wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers */
enum { CACHE=4, LOCAL=6, HTTP=80, HTTPS=443, GEMINI=1965, GOPHER=70 };

int uri_protocol(char *uri);
char *uri_host(char *uri);
int uri_port(char *uri);
char *uri_path(char *uri);
char *uri_normalize(char *relative_uri, char *base_absolute_uri);
