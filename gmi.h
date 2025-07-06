/* Gemini parser and printer */
why_t gmi_onheader(char *line, int *mime, char **redirect);
void gmi_print(char *res, FILE *out);
