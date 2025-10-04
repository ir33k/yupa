#include <stdio.h>
#include <string.h>
#include "main.h"
#include "util.h"
#include "link.h"
#include "mime.h"
#include "gph.h"

int navitems_indexof(char item);
static char *navlabel(char);
static char *navlink(char *line);

static struct {
	char item, *label;
	enum mime mime;
} navitems[] = {
	// Canonical
	'0', "TXT",        MIME_TEXT,   // Text file
	'1', "GPH",        MIME_GPH,    // Gopher submenu
	'2', "CSO",        MIME_TEXT,   // CSO protocol
	'3', "ERROR",      MIME_NONE,   // Error code returned by server
	'4', "BINHEX",     MIME_BINARY, // BinHex-encoded file, for Macintosh
	'5', "DOS",        MIME_TEXT,   // DOS file
	'6', "UUENCODED",  MIME_TEXT,   // uuencoded file
	'7', "SEARCH",     MIME_GPH,    // Gopher full-text search
	'8', "TELNET",     MIME_TEXT,   // Telnet
	'9', "BIN",        MIME_BINARY, // Binary file
	'+', "MIRROR",     MIME_TEXT,   // Mirror or alternate server
	'g', "GIF",        MIME_IMAGE,  // GIF file
	'I', "IMG",        MIME_IMAGE,  // Image file
	'T', "TELNET3270", MIME_TEXT,   // Telnet 3270
	// Gopher+
	':', "BMP",        MIME_IMAGE,  // Bitmap image
	';', "VIDEO",      MIME_VIDEO,  // Movie/video file
	'<', "AUDIO",      MIME_AUDIO,  // Sound file
	// Non-canonical
	'd', "DOC",        MIME_PDF,    // Doc, used alongside .pdf's and .doc's
	'h', "HTML",       MIME_HTML,   // HTML file
	'p', "PNG",        MIME_IMAGE,  // Image file ,especially the png format
	'r', "RTF",        MIME_BINARY, // Document rtf file, rich text format
	's', "WAV",        MIME_AUDIO,  // Sound file, especially the WAV format
	'P', "PDF",        MIME_PDF,    // document pdf file
	'X', "XML",        MIME_TEXT,   // document xml file
	'i', 0,            MIME_NONE,   // Windly used informational message
};

int
navitems_indexof(char item)
{
	int i;

	for (i=0; i<COUNT(navitems); i++)
		if (navitems[i].item == item)
			return i;

	return -1;
}

char *
navlabel(char item)
{
	static char buf[16];
	int i;

	buf[0] = 0;

	i = navitems_indexof(item);

	if (i != -1 && navitems[i].mime != MIME_NONE)
		snprintf(buf, sizeof buf, "(%s) ", navitems[i].label);

	return buf;
}

char *
navlink(char *line)
{
        static char buf[4096];
        char item, path[2048], host[1024];
        int port;

        /* TODO(irek): How Gopher handles links without item type and path?
         * For example link to root hostname page. */
        sscanf(line, "%c%*[^\t]\t%[^\t]\t%[^\t]\t%d", &item, path, host, &port);
        snprintf(buf, sizeof buf, "gopher://%s:%d/%c%s", host, port, item, path);

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

		if (buf[0] == '.')	/* Gopher EOF mark, ed style */
			break;

		nav[0] = 0;
		label = navlabel(buf[0]);

		if (label[0]) {
                        i = link_store(navlink(buf));
			snprintf(nav, sizeof nav, "%u> ", i);
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


enum mime
gph_mime(char *path)
{
	int i;

	if (!path || !path[0])
		return MIME_GPH;

	if (strlen(path) < 2 || path[0] != '/' || path[2] != '/')
		return MIME_GPH;

	for (i=0; i<COUNT(navitems); i++)
		if (navitems[i].item == path[1])
			break;

	return i<COUNT(navitems) ? navitems[i].mime : MIME_TEXT;
}
