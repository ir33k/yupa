/* Gopher protocol. */

#include <errno.h>

enum gph_item {
	/* Canonical types */
	GPH_ITEM_TXT    = '0',  /* Text file */
	GPH_ITEM_GPH    = '1',  /* Gopher submenu */
	GPH_ITEM_CCSO   = '2',  /* CCSO Nameserver */
	GPH_ITEM_ERR    = '3',  /* Error code returned by server */
	GPH_ITEM_BIN16  = '4',  /* BinHex-encoded file (for Macintosh) */
	GPH_ITEM_DOS    = '5',  /* DOS file */
	GPH_ITEM_UUEN   = '6',  /* uuencoded file */
	GPH_ITEM_QUERY  = '7',  /* Gopher full-text search, query */
	GPH_ITEM_TNET   = '8',  /* Telnet */
	GPH_ITEM_BIN    = '9',  /* Binary file */
	GPH_ITEM_MIRR   = '+',  /* Mirror or alternate server */
	GPH_ITEM_GIF    = 'g',  /* GIF file */
	GPH_ITEM_IMG    = 'I',  /* Image file */
	GPH_ITEM_T3270  = 'T',  /* Telnet 3270 */
	/* Gopher+ types */
	GPH_ITEM_BMP    = ':',  /* Bitmap image */
	GPH_ITEM_VIDEO  = ';',  /* Movie/video file */
	GPH_ITEM_AUDIO  = '<',  /* Sound file */
	/* Non-canonical types */
	GPH_ITEM_DOC    = 'd',  /* Doc. Seen used alongside PDF's and .DOC's */
	GPH_ITEM_HTML   = 'h',  /* HTML file */
	GPH_ITEM_PNG    = 'p',  /* Image file "(especially the png format)" */
	GPH_ITEM_RTF    = 'r',  /* Document rtf file ("rich text format") */
	GPH_ITEM_WAV    = 's',  /* Sound file (especially the WAV format) */
	GPH_ITEM_PDF    = 'P',  /* document pdf file */
	GPH_ITEM_XML    = 'X',  /* document xml file */
	GPH_ITEM_INFO   = 'i'   /* Informational message, widely used */
};

/**/
static char *
gph_label(enum gph_item item)
{
	switch(item) {
	case GPH_ITEM_TXT:      return "TXT";
	case GPH_ITEM_GPH:      return "GPH";
	case GPH_ITEM_CCSO:     return "CCSO";
	case GPH_ITEM_ERR:      return "ERR";
	case GPH_ITEM_BIN16:    return "BIN16";
	case GPH_ITEM_DOS:      return "DOS";
	case GPH_ITEM_UUEN:     return "UUEN";
	case GPH_ITEM_QUERY:    return "QUERY";
	case GPH_ITEM_TNET:     return "TNET";
	case GPH_ITEM_BIN:      return "BIN";
	case GPH_ITEM_MIRR:     return "MIRR";
	case GPH_ITEM_GIF:      return "GIF";
	case GPH_ITEM_IMG:      return "IMG";
	case GPH_ITEM_T3270:    return "T3270";
	case GPH_ITEM_BMP:      return "BMP";
	case GPH_ITEM_VIDEO:    return "VIDEO";
	case GPH_ITEM_AUDIO:    return "AUDIO";
	case GPH_ITEM_DOC:      return "DOC";
	case GPH_ITEM_HTML:     return "HTML";
	case GPH_ITEM_PNG:      return "PNG";
	case GPH_ITEM_RTF:      return "RTF";
	case GPH_ITEM_WAV:      return "WAV";
	case GPH_ITEM_PDF:      return "PDF";
	case GPH_ITEM_XML:      return "XML";
	case GPH_ITEM_INFO:     return 0;
	}
	/* I'm assuming that there are no more item types but you never
	   know so here is a guard. */
	assert(0 && "Unhandled item type"); /* Should never happen */
}

/* Assuming that RAW is a file with Gopher submenu, write prettier
 * formatted version to FMT file. */
static void
gph_format(FILE *raw, FILE *fmt)
{
	int n;                  /* Link item index */
	char buf[BSIZ], *label;
	assert(raw);
	assert(fmt);
	rewind(raw);
	n = 1;
	errno = 0;
	while (fgets(buf, sizeof(buf), raw)) {
		if (buf[0] == '.') {    /* Single dot means end of file */
			break;
		}
		buf[strcspn(buf, "\t")] = 0;    /* End string at first tab */
		if ((label = gph_label(buf[0]))) {
			fprintf(fmt, "(%d)\t<%s> ", n++, label);
		}
		fprintf(fmt, "%s\n", &buf[1]);
	}
	if (errno) {
		WARN("fgets:");
	}
}

/* Search in RAW file for the link under INDEX (1 == first link).
 * Return pointer to static string with normalized URI. */
static char *
gph_uri(FILE *raw, int index)
{
	static char uri[BSIZ];
	char c, buf[BSIZ], path[BSIZ], host[BSIZ];
	int port;
	assert(raw);
	assert(index > 0);
	while (fgets(buf, BSIZ, raw)) {
		if (buf[0] == '.') {
			break;
		}
		if (buf[0] == GPH_ITEM_INFO) {
			continue;
		}
		if (--index) {
			continue;
		}
		/* Found */
		sscanf(buf, "%c%*[^\t]\t%[^\t]\t%[^\t]\t%d",
		       &c, path, host, &port);
		if (c == GPH_ITEM_HTML) {
			strcpy(uri, path);
			return uri + 4;
		}
		sprintf(uri, "gopher://%.1024s:%d/%c%.1024s",
			host, port, c, path);
		return uri;
	}
	return 0;
}
