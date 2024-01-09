#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "gph.h"
#include "le.h"
#include "net.h"
#include "uri.h"
#include "util.h"

#define MARGIN 4        // Left margin of FMT formatted Gopher hole

enum type {             // Gopher menu item types
	// Canonical
	T_TXT = '0',    // Text file
	T_GPH = '1',    // Gopher submenu
	T_CSO = '2',    // CSO protocol
	T_ERR = '3',    // Error code returned by server
	T_B16 = '4',    // BinHex-encoded file (for Macintosh)
	T_DOS = '5',    // DOS file
	T_UUE = '6',    // uuencoded file
	T_QRY = '7',    // Gopher full-text search
	T_TEL = '8',    // Telnet
	T_BIN = '9',    // Binary file
	T_MIR = '+',    // Mirror or alternate server
	T_GIF = 'g',    // GIF file
	T_IMG = 'I',    // Image file
	T_TN3 = 'T',    // Telnet 3270
	// Gopher+
	T_BMP = ':',    // Bitmap image
	T_VID = ';',    // Movie/video file
	T_SFX = '<',    // Sound file
	// Non-canonical
	T_DOC = 'd',    // Doc. Seen used alongside PDF's and .DOC's
	T_URL = 'h',    // HTML file
	T_PNG = 'p',    // Image file "(especially the png format)"
	T_RTF = 'r',    // Document rtf file ("rich text format")
	T_WAV = 's',    // Sound file (especially the WAV format)
	T_PDF = 'P',    // document pdf file
	T_XML = 'X',    // document xml file
	T_INF = 'i',    // Informational message, widely used
};

enum kind {             // Kinds of Gopher menu items
	K_NUL = 0,      // Unknown or undefined
	K_NAV,          // Regular navigation menu item
	K_NON = 'i',    // Information type, used for NON menu items
	K_EOF = '.',    // End Of File inficator for Gopher menu file
};

static enum kind
item_kind(enum type item)
{
	switch (item) {
	case T_TXT: case T_GPH: case T_CSO: case T_ERR: case T_B16:
	case T_DOS: case T_UUE: case T_QRY: case T_TEL: case T_BIN:
	case T_MIR: case T_GIF: case T_IMG: case T_TN3: case T_BMP:
	case T_VID: case T_SFX: case T_DOC: case T_URL: case T_PNG:
	case T_RTF: case T_WAV: case T_PDF: case T_XML:
		return K_NAV;
	case T_INF:
		return K_NON;
	}
	if ((int)item == K_EOF) {
		return K_EOF;
	}
	return K_NUL;
}

static char *
item_label(enum type item)
{
	switch (item) {
	case T_TXT: return "TEXT";
	case T_GPH: return "GPH";
	case T_CSO: return "CSO";
	case T_ERR: return "ERROR";
	case T_B16: return "BINHEX";
	case T_DOS: return "DOS";
	case T_UUE: return "UUENCODED";
	case T_QRY: return "SEARCH";
	case T_TEL: return "TELNET";
	case T_BIN: return "BIN";
	case T_MIR: return "MIRROR";
	case T_GIF: return "GIF";
	case T_IMG: return "IMG";
	case T_TN3: return "TN3270";
	case T_BMP: return "BMP";
	case T_VID: return "VIDEO";
	case T_SFX: return "AUDIO";
	case T_DOC: return "DOC";
	case T_URL: return "HTML";
	case T_PNG: return "PNG";
	case T_RTF: return "RTF";
	case T_WAV: return "WAV";
	case T_PDF: return "PDF";
	case T_XML: return "XML";
	case T_INF: return "INFO";
	}
	return "???";
}

FILE *
gph_req(FILE *raw, FILE *fmt, char *uri)
{
	int sfd, port;
	char buf[4096], *tmp, *host, *path, item='1';
	FILE *show;
	ssize_t ssiz;
	assert(raw);
	assert(fmt);
	assert(uri);
	host = uri_host(uri);
	port = uri_port(uri);
	path = uri_path(uri);
	if (!port) {
		port = URI_GOPHER;
	}
	if (path && path[1]) {
		item = path[1];
		path += 2;
	}
	if (item == '7') {
		// TODO(irek): Make sure that there is a way to cancel.
		fputs("enter search query: ", stdout);
		fflush(stdout);
		fgets(buf, sizeof(buf), stdin);
		buf[strlen(buf)-1] = 0;
		if (!buf[0]) { // Empty search
			return 0;
		}
		tmp = JOIN(path, "\t", buf);
		path = tmp;
	}
	if ((sfd = req(host, port, path)) == 0) {
		printf("Request '%s' failed\n", uri);
		return 0;
	}
	while ((ssiz = recv(sfd, buf, sizeof(buf), 0)) > 0) {
		if (fwrite(buf, 1, ssiz, raw) != (size_t)ssiz) {
			ERR("fwrite '%s':", uri);
		}
	}
	if (ssiz < 0) {
		ERR("recv '%s':", uri);
	}
	if (close(sfd)) {
		ERR("close '%s' %d:", uri, sfd);
	}
	switch (item) {
	case '0':
		show = raw;
		break;
	case '1':
	case '7':
		show = fmt;
		rewind(raw);
		gph_fmt(raw, fmt);
		break;
	default:
		show = 0;
	}
	return show;
}

void
gph_fmt(FILE *body, FILE *dst)
{
	int link=1;
	char *bp, buf[1024], nav[8], label[16];
	assert(body);
	assert(dst);
	while ((bp = fgets(buf, sizeof(buf), body))) {
		bp[strcspn(bp, "\t")] = 0; // End string on first tab
		*nav = 0;
		*label = 0;
		switch (item_kind(*bp)) {
		case K_EOF: return;
		case K_NUL: break;
		case K_NAV:
			snprintf(nav, sizeof(nav), "%d: ", link++);
			snprintf(label, sizeof(label), "(%s) ", item_label(*bp));
			// Fallthrough
		case K_NON:
			bp++;   // Skip item type character
		}
		fprintf(dst, "%-*s%s%s\n", MARGIN, nav, label, bp);
	}
}

char *
gph_uri(FILE *body, int index)
{
	static char uri[URI_SIZ];
	char c, buf[4096], path[1024], host[1024];
	int port;
	assert(body);
	assert(index > 0);
	while (fgets(buf, sizeof(buf), body)) {
		switch (item_kind(*buf)) {
		case K_NAV: break;
		case K_NON: continue;
		case K_NUL: continue;
		case K_EOF: return 0;
		}
		if (--index) {
			continue;
		}
		// Found
		sscanf(buf, "%c%*[^\t]\t%[^\t]\t%[^\t]\t%d",
		       &c, path, host, &port);
		if (c == T_URL) {
			if (strlen(path) < 5) {
				return 0;
			}
			strcpy(uri, path);
			return uri + 4;
		}
#pragma GCC diagnostic ignored "-Wformat-truncation=" // Trust me bro
		snprintf(uri, sizeof(uri), "gopher://%s:%d/%c%s",
			 host, port, c, path);
#pragma GCC diagnostic pop
		return uri;
	}
	return 0;
}
