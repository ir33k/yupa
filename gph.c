#include <stdio.h>
#include <string.h>

#include "main.h"
#include "gph.h"

typedef struct nav	Nav;		/* Gopher navigation item */

struct nav {
	char	item;
	char*	label;
	Mime	mime;
};

static Nav*	navs_get(char item);
static char*	navlabel(char);
static char*	navlink(char *line);

static Nav navs[] = {
	/* Canonical */
	'0',	"TXT",		MIME_TEXT,	/* Text file */
	'1',	"",		MIME_GPH,	/* Gopher submenu */
	'2',	"CSO",		MIME_TEXT,	/* CSO protocol */
	'3',	"ERROR",	0,		/* Server error message */
	'4',	"BINHEX",	MIME_BINARY,	/* BinHex Macintosh file */
	'5',	"DOS",		MIME_TEXT,	/* DOS file */
	'6',	"UUENCODED",	MIME_TEXT,	/* uuencoded file */
	'7',	"SEARCH",	MIME_GPH,	/* Gopher full-text search */
	'8',	"TELNET",	MIME_TEXT,	/* Telnet */
	'9',	"BIN",		MIME_BINARY,	/* Binary file */
	'+',	"MIRROR",	MIME_TEXT,	/* Mirror or alternate server */
	'g',	"GIF",		MIME_IMAGE,	/* GIF file */
	'I',	"IMG",		MIME_IMAGE,	/* Image file */
	'T',	"TELNET3270",	MIME_TEXT,	/* Telnet 3270 */
	/* Gopher+ */
	':',	"BMP",		MIME_IMAGE,	/* Bitmap image */
	';',	"VIDEO",	MIME_VIDEO,	/* Movie/video file */
	'<',	"AUDIO",	MIME_AUDIO,	/* Sound file */
	/* Non-canonical */
	'd',	"DOC",		MIME_PDF,	/* PDF's and DOC's */
	'h',	"HTML",		MIME_HTML,	/* HTML file */
	'p',	"PNG",		MIME_IMAGE,	/* Image file, mainly png */
	'r',	"RTF",		MIME_BINARY,	/* RTF (rich text format) */
	's',	"WAV",		MIME_AUDIO,	/* Sound file, mainly WAV */
	'P',	"PDF",		MIME_PDF,	/* PDF */
	'X',	"XML",		MIME_TEXT,	/* XML */
	'i',	0,		0,		/* Windly used info message */
};

Nav*
navs_get(char item)
{
	int i;

	for (i=0; i<LENGTH(navs); i++)
		if (navs[i].item == item)
			return navs + i;

	return 0;
}

char *
navlabel(char item)
{
	static char buf[16];
	Nav *nav;

	nav = navs_get(item);

	if (!nav || !nav->mime)
		return 0;

	if (nav->label[0] == 0)
		return "";

	snprintf(buf, sizeof buf, "(%s) ", nav->label);
	return buf;
}

char *
navlink(char *line)
{
        static char buf[4096], path[4096];
        char item, host[1024];
        int port;

        sscanf(line, "%c%*[^\t]\t%[^\t]\t%[^\t]\t%d", &item, path, host, &port);

	if (item == 'h')		/* HTTP */
		return path+4;

	snprintf(buf, sizeof buf, "gopher://%s:%d/%c%s",
		 host, port, item, path);

        return buf;
}

void
gph_print(FILE *res, FILE *out)
{
	char buf[4096], nav[16], *label;
	unsigned i, n;

	while (fgets(buf, sizeof buf, res)) {
		n = strlen(buf);
		if (n && buf[n-1] == '\n')
			buf[--n] = 0;

		if (!strcmp(buf, "."))	/* Gopher EOF mark, ed style */
			break;

		nav[0] = 0;
		label = navlabel(buf[0]);

		if (label) {
                        i = link_store(navlink(buf));
			snprintf(nav, sizeof nav, "%u> ", i);
                } else {
			label = "";
		}

                n = strcspn(buf, "\t");
		fprintf(out, "%-*s%s%.*s\n", envmargin, nav, label, n, buf+1);
	}
	fprintf(out, "\n");
}

char *
gph_search(char *path)
{
	static char buf[4096];

	if (path[1] != '7')
		return 0;
	
	printf("search: ");

	fgets(buf+1, (sizeof buf) -1, stdin);
	trim(buf+1);
	buf[0] = '\t';

	return buf;
}

Mime
gph_mime(char *path)
{
	Nav *nav;

	if (!path || strlen(path) < 2 || path[0] != '/' || path[2] != '/')
		return MIME_GPH;

	if (!(nav = navs_get(path[1])))
		return MIME_TEXT;

	return nav->mime;
}
