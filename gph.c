/* Gopher protocol */

#include <stdio.h>
#include <string.h>
#include "main.h"

typedef struct nav	Nav;		/* Gopher navigation line type */

struct nav {
	char	type;
	char*	label;
	Mime	mime;
};

static Nav*	navs_get(char type);

static Nav navs[] = {
	/* Canonical */
	'0',	"TXT",		MIME_TEXT,	/* Text file */
	'1',	0,		MIME_GPH,	/* Gopher submenu */
	'2',	"CSO",		MIME_TEXT,	/* CSO protocol */
	'3',	0,		0,		/* Server error message */
	'4',	"BINHEX",	MIME_BINARY,	/* BinHex Macintosh file */
	'5',	"DOS",		MIME_TEXT,	/* DOS file */
	'6',	"UUENCODED",	MIME_TEXT,	/* uuencoded file */
	'7',	"SEARCH",	MIME_GPH,	/* Gopher full-text search */
	'8',	"TELNET",	MIME_TEXT,	/* Telnet */
	'9',	"BIN",		MIME_BINARY,	/* Binary file */
	'+',	"MIRROR",	MIME_TEXT,	/* Mirror or alternate server */
	'g',	"GIF",		MIME_BINARY,	/* GIF file */
	'I',	"IMG",		MIME_BINARY,	/* Image file */
	'T',	"TELNET3270",	MIME_TEXT,	/* Telnet 3270 */
	/* Gopher+ */
	':',	"BMP",		MIME_BINARY,	/* Bitmap image */
	';',	"VIDEO",	MIME_BINARY,	/* Movie/video file */
	'<',	"AUDIO",	MIME_BINARY,	/* Sound file */
	/* Non-canonical */
	'd',	"DOC",		MIME_BINARY,	/* PDF's and DOC's */
	'h',	"HTML",		MIME_TEXT,	/* HTML file */
	'p',	"PNG",		MIME_BINARY,	/* Image file, mainly png */
	'r',	"RTF",		MIME_BINARY,	/* RTF (rich text format) */
	's',	"WAV",		MIME_BINARY,	/* Sound file, mainly WAV */
	'P',	"PDF",		MIME_BINARY,	/* PDF */
	'X',	"XML",		MIME_TEXT,	/* XML */
	'i',	0,		0,		/* Windly used info message */
	/* Fallback */
	0,	"UNKNOWN",	MIME_BINARY,	/* Assume binary for unknown */
};

Nav*
navs_get(char type)
{
	int i;

	for (i=0; i<LENGTH(navs) -1; i++)	/* -1 makes UNKNOWN default */
		if (navs[i].type == type)
			break;

	return navs + i;
}

void
gph_print(FILE *res, FILE *out)
{
	char buf[4096], uri[4096], index[16], label[16];
	char type, name[256], selector[256], host[256];
        int i, port;
	Nav *nav;

	while (fgets(buf, sizeof buf, res)) {
		if (!strcmp(buf, ".\r\n"))
			break;

		index[0] = 0;
		label[0] = 0;
		name[0] = 0;

		sscanf(buf, "%c%[^\t] %s %s %d",
		       &type, name, selector, host, &port);

		if (type == 'h')
			snprintf(uri, sizeof uri, "%s", selector + 4);
		else
			snprintf(uri, sizeof uri, "gopher://%s:%d/%c%s",
				 host, port, type, selector);

		nav = navs_get(type);

		if (nav->mime) {
                        i = link_store(uri);
			snprintf(index, sizeof index, "%d ", i);
		}

		if (nav->label)
			snprintf(label, sizeof label, " (%s)", nav->label);

		fprintf(out, "%-*s%s%s\n", envmargin, index, name, label);
	}
	fprintf(out, "\n");
}

char *
gph_search(char *path)
{
	static char buf[4096] = "\t";

	if (path[1] != '7')
		return 0;
	
	printf("search: ");

	fgets(buf +1, (sizeof buf) -1, stdin);
	trimr(buf);

	return buf;
}

Mime
gph_mime(char *path)
{
	Nav *nav;

	if (!path || strlen(path) < 2 || path[0] != '/' || path[2] != '/')
		return MIME_GPH;

	nav = navs_get(path[1]);

	return nav->mime;
}
