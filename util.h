#define SIZE(array) (int)((sizeof array) / (sizeof (array)[0]))

typedef char *why_t;

why_t tellme(char *, ...);
char *join(char *, char *);
char *resolvepath(char *);
char *fmalloc(char *path);
char *eachline(char **);
char *eachword(char **);
char *triml(char *);
void trimr(char *);
char *trim(char *);
char *cp(char *, char *);
int startswith(char *, char *prefix);
