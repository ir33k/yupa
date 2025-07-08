#define COUNT(array) (int)((sizeof array) / (sizeof (array)[0]))

typedef char *why_t;

why_t tellme(char *, ...);
char *join(char *, char *);
char *resolvepath(char *);
char *eachword(char **);
char *trim(char *);
why_t fcp(FILE *from, FILE *to);
why_t cp(char *from, char *to);
int startswith(char *, char *prefix);
