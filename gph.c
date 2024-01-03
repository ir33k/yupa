// Gopher protocol.

// For ITEM type of Gopher submenu line return pointer to static
// string with human readable label.  Return NULL for unknown type.
static char *
gph_label(char item)
{
	switch(item) {
	// Canonical types
	case '0': return "TXT";         // Text file
	case '1': return "GPH";         // Gopher submenu
	case '2': return "CCSO";        // CCSO Nameserver
	case '3': return "ERROR";       // Error code returned by server
	case '4': return "BIN16";       // BinHex-encoded file (for Macintosh)
	case '5': return "DOS";         // DOS file
	case '6': return "UUEN";        // uuencoded file
	case '7': return "QUERY";       // Gopher full-text search, query
	case '8': return "TNET";        // Telnet
	case '9': return "BIN";         // Binary file
	case '+': return "MIRR";        // Mirror or alternate server
	case 'g': return "GIF";         // GIF file
	case 'I': return "IMG";         // Image file
	case 'T': return "T3270";       // Telnet 3270
	// Gopher+ types
	case ':': return "BMP";         // Bitmap image
	case ';': return "VIDEO";       // Movie/video file
	case '<': return "AUDIO";       // Sound file
	// Non-canonical types
	case 'd': return "DOC";         // Doc. Seen used alongside PDF's and DOC's
	case 'h': return "HTML";        // HTML file
	case 'p': return "PNG";         // Image file "(especially the png format)"
	case 'r': return "RTF";         // Document rtf file ("rich text format")
	case 's': return "WAV";         // Sound file (especially the WAV format)
	case 'P': return "PDF";         // document pdf file
	case 'X': return "XML";         // document xml file
	case 'i': return "";            // Informational message, widely used
	}
	return 0;                       // Unknown type
}

// Assuming that BODY is an open file with Gopher submenu, write
// prettier formatted version to open FMT file.
static void
gph_format(FILE *body, FILE *fmt)
{
	static const int margin = 4;    // Left margin
	int link = 1;                   // Link index
	char buf[BSIZ], tmp[8], *label;
	assert(body);
	assert(fmt);
	while (fgets(buf, sizeof(buf), body)) {
		if (*buf == '.') {              // Single dot means end of file
			break;
		}
		buf[strcspn(buf, "\t")] = 0;    // End string at first tab
		label = gph_label(buf[0]);
		if (!label) {                   // Unknown label
			fprintf(fmt, "%-*s<\?\?\?> %s\n", margin, "", buf);
		} else if (*label) {            // Link with label
			snprintf(tmp, sizeof(tmp), "%d: ", link++);
			fprintf(fmt, "%-*s<%s> %s\n", margin, tmp, label, buf+1);
		} else {                        // Empty label
			fprintf(fmt, "%-*s%s\n", margin, "", buf+1);
		}
	}
}

// Search in BODY open file for the link under INDEX (1 == first
// link).  Return pointer to static string with normalized URI.
static char *
gph_uri(FILE *body, int index)
{
	static char uri[URI_SIZ];
	char c, buf[BSIZ], path[URI_SIZ], host[URI_SIZ], *label;
	int port;
	assert(body);
	assert(index > 0);
	while (fgets(buf, sizeof(buf), body)) {
		if (buf[0] == '.') {
			break;
		}
		label = gph_label(buf[0]);
		if (!label || !*label || --index) {
			continue;
		}
		// Found
		sscanf(buf, "%c%*[^\t]\t%[^\t]\t%[^\t]\t%d",
		       &c, path, host, &port);
		if (c == 'h') {
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
