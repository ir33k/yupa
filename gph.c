#include <stdio.h>
#include <string.h>
#include "main.h"
#include "util.h"
#include "link.h"
#include "mime.h"
#include "gph.h"

static char *navlabel(char);
static char *navlink(char *line);

static struct {
	char item, *label;
	enum mime mime;		// 0 for non nav menu items
} navitems[] = {
	// Canonical
	'0',    "TEXT",         TEXT,   // Text file
	'1',    "GPH",          GPH,    // Gopher submenu
	'2',    "CSO",          TEXT,   // CSO protocol
	'3',    "ERROR",        0,      // Error code returned by server
	'4',    "BINHEX",       BINARY, // BinHex-encoded file, for Macintosh
	'5',    "DOS",          TEXT,   // DOS file
	'6',    "UUENCODED",    TEXT,   // uuencoded file
	'7',    "SEARCH",       GPH,    // Gopher full-text search
	'8',    "TELNET",       TEXT,   // Telnet
	'9',    "BIN",          BINARY, // Binary file
	'+',    "MIRROR",       TEXT,   // Mirror or alternate server
	'g',    "GIF",          IMAGE,  // GIF file
	'I',    "IMG",          IMAGE,  // Image file
	'T',    "TELNET3270",   TEXT,   // Telnet 3270
	// Gopher+
	':',    "BMP",          IMAGE,  // Bitmap image
	';',    "VIDEO",        VIDEO,  // Movie/video file
	'<',    "AUDIO",        AUDIO,  // Sound file
	// Non-canonical
	'd',    "DOC",          PDF,    // Doc. Seen used alongside PDF's and .DOC's
	'h',    "HTML",         HTML,   // HTML file
	'p',    "PNG",          IMAGE,  // Image file ,especially the png format
	'r',    "RTF",          BINARY, // Document rtf file, rich text format
	's',    "WAV",          AUDIO,  // Sound file, especially the WAV format
	'P',    "PDF",          PDF,    // document pdf file
	'X',    "XML",          TEXT,   // document xml file
	'i',    0,              0,      // Windly used informational message
};

char *
navlabel(char item)
{
	static char buf[16];
	int i;

	buf[0] = 0;

	for (i=0; i<SIZE(navitems); i++)
		if (navitems[i].item == item)
			break;

	if (i<SIZE(navitems) && navitems[i].mime)
		snprintf(buf, sizeof buf, "(%s) ", navitems[i].label);

	return buf;
}

char *
navlink(char *line)
{
        static char buf[4096];
        char item, path[1024], host[1024];
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
		return GPH;

	if (strlen(path) < 2 || path[0] != '/' || path[2] != '/')
		return GPH;

	for (i=0; i<SIZE(navitems); i++)
		if (navitems[i].item == path[1])
			break;

	return i<SIZE(navitems) ? navitems[i].mime : TEXT;
}
