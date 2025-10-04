/* HTML parser and printer */
void html_print(FILE *res, FILE *out);
why_t html_onheader(FILE *res, enum mime *mime, char **redirect);
