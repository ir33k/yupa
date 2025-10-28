#define LENGTH(a)	(int)((sizeof(a)) / (sizeof(a)[0]))

enum mime { MIME_NONE, MIME_BINARY, MIME_TEXT, MIME_GPH, MIME_GMI };

typedef char*		Why;
typedef enum mime	Mime;

extern int	envmargin;
extern int	envwidth;

char*	eachword	(char **str, char *separators);
char*	triml		(char*);
void	trimr		(char*);
char*	trim		(char*);
int	starts		(char *str, char *with);
int	link_store	(char*);

void	txt_print	(FILE *res, FILE *out);

void	gph_print	(FILE *res, FILE *out);
char*	gph_search	(char *path);
Mime	gph_mime	(char *path);

void	gmi_print	(FILE *res, FILE *out);
char*	gmi_search	(char *header);
Why	gmi_onheader	(FILE *res, char **header, char **redirect);
