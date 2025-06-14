#include <stdio.h>
#include <string.h>
#include "gph.h"
#include "util.h"

#define MARGIN 4        /* Left margin of FMT formatted Gopher hole */

static char *navlabel(char);

char *
navlabel(char item)
{
	switch (item) {
	/* Canonical */
	case '0': return "(TEXT) ";      /* Text file */
	case '1': return "(GPH) ";       /* Gopher submenu */
	case '2': return "(CSO) ";       /* CSO protocol */
	case '3': return "(ERROR) ";     /* Error code returned by server */
	case '4': return "(BINHEX) ";    /* BinHex-encoded file (for Macintosh) */
	case '5': return "(DOS) ";       /* DOS file */
	case '6': return "(UUENCODED) "; /* uuencoded file */
	case '7': return "(SEARCH) ";    /* Gopher full-text search */
	case '8': return "(TELNET) ";    /* Telnet */
	case '9': return "(BIN) ";       /* Binary file */
	case '+': return "(MIRROR) ";    /* Mirror or alternate server */
	case 'g': return "(GIF) ";       /* GIF file */
	case 'I': return "(IMG) ";       /* Image file */
	case 'T': return "(TN3270) ";    /* Telnet 3270 */
	/* Gopher+ */
	case ':': return "(BMP) ";       /* Bitmap image */
	case ';': return "(VIDEO) ";     /* Movie/video file */
	case '<': return "(AUDIO) ";     /* Sound file */
	/* Non-canonical */
	case 'd': return "(DOC) ";       /* Doc. Seen used alongside PDF's and .DOC's */
	case 'h': return "(HTML) ";      /* HTML file */
	case 'p': return "(PNG) ";       /* Image file "(especially the png format)" */
	case 'r': return "(RTF) ";       /* Document rtf file ("rich text format") */
	case 's': return "(WAV) ";       /* Sound file (especially the WAV format) */
	case 'P': return "(PDF) ";       /* document pdf file */
	case 'X': return "(XML) ";       /* document xml file */
	}
	/* Everything else is not a navigatoin item, like
	 * 'i' (Informational message, widely used) */
	return "";
}

void
gph_print(char *in, FILE *out)
{
	char *line, nav[16], *label;
	int i;

	i = 0;
	while ((line = online(&in))) {
		if (line[0] == '.')
			break;

		nav[0] = 0;
		label = navlabel(line[0]);
		line++;

		if (label[0])
			snprintf(nav, sizeof nav, "%d: ", ++i);

		line[strcspn(line, "\t")] = 0;	/* End string on first tab */

		fprintf(out, "%-*s%s%s\n", MARGIN, nav, label, line);
	}
}
