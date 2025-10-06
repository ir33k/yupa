/* Gemini parser and printer */
char*	gmi_search	(char *header);
Err	gmi_onheader	(FILE *res, char **header, char **redirect);
void	gmi_print	(FILE *res, FILE *out);
