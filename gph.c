// Gopher protocol.

enum gph_type {         // Gopher menu item types
	// Canonical
	GPH_TXT = '0',  // Text file
	GPH_GPH = '1',  // Gopher submenu
	GPH_CSO = '2',  // CSO protocol
	GPH_ERR = '3',  // Error code returned by server
	GPH_B16 = '4',  // BinHex-encoded file (for Macintosh)
	GPH_DOS = '5',  // DOS file
	GPH_UUE = '6',  // uuencoded file
	GPH_QRY = '7',  // Gopher full-text search
	GPH_TEL = '8',  // Telnet
	GPH_BIN = '9',  // Binary file
	GPH_MIR = '+',  // Mirror or alternate server
	GPH_GIF = 'g',  // GIF file
	GPH_IMG = 'I',  // Image file
	GPH_TN3 = 'T',  // Telnet 3270
	// Gopher+
	GPH_BMP = ':',  // Bitmap image
	GPH_VID = ';',  // Movie/video file
	GPH_SFX = '<',  // Sound file
	// Non-canonical
	GPH_DOC = 'd',  // Doc. Seen used alongside PDF's and .DOC's
	GPH_URL = 'h',  // HTML file
	GPH_PNG = 'p',  // Image file "(especially the png format)"
	GPH_RTF = 'r',  // Document rtf file ("rich text format")
	GPH_WAV = 's',  // Sound file (especially the WAV format)
	GPH_PDF = 'P',  // document pdf file
	GPH_XML = 'X',  // document xml file
	GPH_INF = 'i',  // Informational message, widely used
};

enum gph_kind {         // Kinds of Gopher menu items
	GPH_NUL = 0,    // Unknown or undefined
	GPH_NAV,        // Regular navigation menu item
	GPH_NON = 'i',  // Information type, used for NON menu items
	GPH_EOF = '.',  // End Of File inficator for Gopher menu file
};

static enum gph_kind
gph_kind(int item)
{
	switch (item) {
	case GPH_TXT: case GPH_GPH: case GPH_CSO: case GPH_ERR:
	case GPH_B16: case GPH_DOS: case GPH_UUE: case GPH_QRY:
	case GPH_TEL: case GPH_BIN: case GPH_MIR: case GPH_GIF:
	case GPH_IMG: case GPH_TN3: case GPH_BMP: case GPH_VID:
	case GPH_SFX: case GPH_DOC: case GPH_URL: case GPH_PNG:
	case GPH_RTF: case GPH_WAV: case GPH_PDF: case GPH_XML:
		return GPH_NAV;
	case GPH_INF:
		return GPH_NON;
	case GPH_EOF:
		return GPH_EOF;
	}
	return GPH_NUL;
}

static char *
gph_str(enum gph_type item)
{
	switch (item) {
	case GPH_TXT: return "TEXT";
	case GPH_GPH: return "GPH";
	case GPH_CSO: return "CSO";
	case GPH_ERR: return "ERROR";
	case GPH_B16: return "BINHEX";
	case GPH_DOS: return "DOS";
	case GPH_UUE: return "UUENCODED";
	case GPH_QRY: return "SEARCH";
	case GPH_TEL: return "TELNET";
	case GPH_BIN: return "BIN";
	case GPH_MIR: return "MIRROR";
	case GPH_GIF: return "GIF";
	case GPH_IMG: return "IMG";
	case GPH_TN3: return "TN3270";
	case GPH_BMP: return "BMP";
	case GPH_VID: return "VIDEO";
	case GPH_SFX: return "AUDIO";
	case GPH_DOC: return "DOC";
	case GPH_URL: return "HTML";
	case GPH_PNG: return "PNG";
	case GPH_RTF: return "RTF";
	case GPH_WAV: return "WAV";
	case GPH_PDF: return "PDF";
	case GPH_XML: return "XML";
	case GPH_INF: return "INFO";
	}
	return "???";
}

// Assuming that BODY is an open file with Gopher submenu, write
// prettier formatted version to open FMT file.
static void
gph_format(FILE *body, FILE *fmt)
{
	static const int margin=4;      // Left margin
	int link=1;
	char *bp, buf[1024], nav[8], str[16];
	assert(body);
	assert(fmt);
	while ((bp = fgets(buf, sizeof(buf), body))) {
		bp[strcspn(bp, "\t")] = 0; // End string on first tab
		*nav = 0;
		*str = 0;
		switch (gph_kind(*bp)) {
		case GPH_EOF: return;
		case GPH_NUL: break;
		case GPH_NAV:
			snprintf(nav, sizeof(nav), "%d: ", link++);
			snprintf(str, sizeof(str), "(%s) ", gph_str(*bp));
			// Fallthrough
		case GPH_NON:
			bp++;   // Skip first item type character
		}
		fprintf(fmt, "%-*s%s%s\n", margin, nav, str, bp);
	}
}

// Search in BODY open file for the link under INDEX (1 == first
// link).  Return pointer to static string with normalized URI.
static char *
gph_uri(FILE *body, int index)
{
	static char uri[URI_SIZ];
	char c, buf[4056], path[1024], host[1024];
	int port;
	assert(body);
	assert(index > 0);
	while (fgets(buf, sizeof(buf), body)) {
		switch (gph_kind(*buf)) {
		case GPH_EOF: return 0;
		case GPH_NAV: break;
		case GPH_NON: continue;
		case GPH_NUL: continue;
		default:      continue;
		}
		if (--index) {
			continue;
		}
		// Found
		sscanf(buf, "%c%*[^\t]\t%[^\t]\t%[^\t]\t%d",
		       &c, path, host, &port);
		if (c == GPH_URL) {
			strcpy(uri, path);
			return uri + 4;
		}
#pragma GCC diagnostic ignored "-Wformat-truncation="
		snprintf(uri, sizeof(uri), "gopher://%s:%d/%c%s",
			 host, port, c, path);
#pragma GCC diagnostic pop
		return uri;
	}
	return 0;
}
