/* Gemini parser and printer */
char *gmi_search(char *header);
why_t gmi_onheader(FILE *res, int *mime, char **redirect);
void gmi_print(FILE *res, FILE *out);
