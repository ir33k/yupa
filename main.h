#define LENGTH(a)	(int)((sizeof(a)) / (sizeof(a)[0]))

enum mime { MIME_NONE, MIME_BINARY, MIME_TEXT, MIME_GPH, MIME_GMI };

typedef char*		Why;
typedef enum mime	Mime;

extern int	envmargin;
extern int	envwidth;

char*	eachword	(char**);
char*	triml		(char*);
void	trimr		(char*);
char*	trim		(char*);
int	starts		(char *str, char *with);
int	link_store	(char*);
