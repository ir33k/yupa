#include <stdio.h>
#include <string.h>
#include "util.h"
#include "link.h"
#include "main.h"
#include "gph.h"

static char *navlabel(char);
char *navlink(char *line);

char *
navlabel(char item)
{
	switch (item) {
	/* Canonical */
	case '0': return "(TEXT) ";       /* Text file */
	case '1': return "(GPH) ";        /* Gopher submenu */
	case '2': return "(CSO) ";        /* CSO protocol */
	case '3': return "(ERROR) ";      /* Error code returned by server */
	case '4': return "(BINHEX) ";     /* BinHex-encoded file, for Macintosh */
	case '5': return "(DOS) ";        /* DOS file */
	case '6': return "(UUENCODED) ";  /* uuencoded file */
	case '7': return "(SEARCH) ";     /* Gopher full-text search */
	case '8': return "(TELNET) ";     /* Telnet */
	case '9': return "(BIN) ";        /* Binary file */
	case '+': return "(MIRROR) ";     /* Mirror or alternate server */
	case 'g': return "(GIF) ";        /* GIF file */
	case 'I': return "(IMG) ";        /* Image file */
	case 'T': return "(TELNET3270) "; /* Telnet 3270 */
	/* Gopher+ */
	case ':': return "(BMP) ";        /* Bitmap image */
	case ';': return "(VIDEO) ";      /* Movie/video file */
	case '<': return "(AUDIO) ";      /* Sound file */
	/* Non-canonical */
	case 'd': return "(DOC) ";        /* Doc. Seen used alongside PDF's and .DOC's */
	case 'h': return "(HTML) ";       /* HTML file */
	case 'p': return "(PNG) ";        /* Image file ,especially the png format */
	case 'r': return "(RTF) ";        /* Document rtf file, rich text format */
	case 's': return "(WAV) ";        /* Sound file, especially the WAV format */
	case 'P': return "(PDF) ";        /* document pdf file */
	case 'X': return "(XML) ";        /* document xml file */
	}
	/* Everything else is not a navigatoin item, like
	 * 'i' (Informational message, widely used) */
	return "";
}

char *
navlink(char *line)
{
        static char buf[4096];
        char item, path[1024], host[1024];
        int port;

        sscanf(line, "%c%*[^\t]\t%[^\t]\t%[^\t]\t%d", &item, path, host, &port);
        snprintf(buf, sizeof buf, "gopher://%s:%d/%c%s", host, port, item, path);

        return buf;
}

void
gph_print(char *res, FILE *out)
{
	char *line, nav[16], *label;
	unsigned i, n;

	while ((line = eachline(&res))) {
		if (line[0] == '.')	/* Gopher EOF mark, ed style */
			break;

		nav[0] = 0;
		label = navlabel(line[0]);

		if (label[0]) {
                        i = link_store(navlink(line));
			snprintf(nav, sizeof nav, "%u> ", i);
                }

                n = strcspn(line, "\t");
		fprintf(out, "%-*s%s%.*s\n", envmargin, nav, label, n, line+1);
	}
}
