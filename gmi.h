/* Gemini parser and printer */
char *gmi_search(char *header);
why_t gmi_onheader(FILE *res, enum mime *mime, char **redirect);
void gmi_print(FILE *res, FILE *out);
