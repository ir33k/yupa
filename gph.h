/* Gopher protocol */
void gph_print(FILE *res, FILE *out);
char *gph_search(char *path);
enum mime gph_mime(char *path);
