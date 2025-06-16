#define SIZE(array) (int)((sizeof array) / (sizeof (array)[0]))

char *fmalloc(FILE *);
char *eachline(char **);
char *eachword(char **);
char *triml(char *);
void trimr(char *);
