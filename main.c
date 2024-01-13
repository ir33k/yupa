#define NAME    "Yupa"
#define VERSION "v3.1"
#define AUTHOR  "irek@gabr.pl"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "lib/arg.h"
#include "lib/le.h"
#include "lib/nav.h"
#include "lib/tab.h"
#include "lib/uri.h"
#include "lib/util.h"
// Protocols
#include "lib/fsp.h"
#include "lib/gmi.h"
#include "lib/gph.h"

#define PROTOCOL URI_GOPHER     // Default protocol

static struct tab  s_tab={0};   // Browser tabs
static char       *s_pager;     // Pager command
char              *argv0;       // First program arg, for arg.h

static const char *s_help =
	NAME " " VERSION " by " AUTHOR "\n"
	"\n"
	"Multi protocol CLI browser with tabs and browsing history.\n"
	"Supports basics of gopher://, gemini:// and file:// protocols.\n"
	"Browse by inserting absolute URI or link index from current page.\n"
	"Press RETURN to open navigation menu or insert command upfront.\n"
	"Prompt indicate (current_tab_number/number_of_all_tabs).\n"
	"Run program with -h flag to read about arguments and env vars.\n"
	"\n";

// Print usage help message.
static void
usage(void)
{
	printf("usage: %s [-v] [-h] [uri..]\n"
	       "\n"
	       "	-v	Print version.\n"
	       "	-h	Print this help message.\n"
	       "	[uri..]	List of URIs to open on startup.\n"
	       "env	PAGER	Pager cmd (less -XI).\n"
	       , argv0);
}

// Get URI under INDEX link (1 based) from currently open tab.
static char *
onlink(int index)
{
	enum uri protocol;
	char *uri=0, *filename;
	FILE *raw;
	if (index < 1) {
		return 0;
	}
	protocol = s_tab.open->protocol;
	filename = s_tab.open->raw;
	uri = past_get(s_tab.open->past, 0);
	if (!(raw = fopen(filename, "r"))) {
		ERR("fopen %s:", filename);
	}
	switch (protocol) {
	case URI_GOPHER: uri = gph_uri(raw, index); break;
	case URI_GEMINI: uri = gmi_uri(raw, index); break;
	case URI_FILE:   uri = fsp_uri(uri, raw, index); break;
	case URI_FTP:
	case URI_SSH:
	case URI_FINGER:
	case URI_HTTP:
	case URI_HTTPS:
	case URI_NUL:
	default:
		WARN("Unsupported protocol %d '%s'", protocol,
		     uri_protocol_str(protocol));
		uri = 0;
	}
	if (fclose(raw) == EOF) {
		ERR("fclose %s:", filename);
	}
	return uri;
}

//
static int
onuri(char *uri, int save)
{
	enum net_res res;
	enum uri protocol;
	char *old, new[URI_SZ];
	int port;
	FILE *raw, *fmt;
	if (!uri || !uri[0]) {
		return 0;
	}
	assert(strlen(uri) <= URI_SZ);
	if (!uri_abs(uri)) {    // Relative URI.
		old = past_get(s_tab.open->past, 0);
		if (*uri != '/') {
			uri = JOIN(uri_path(old), uri);
		}
		uri = uri_norm(s_tab.open->protocol, uri_host(old),
			       uri_port(old), uri);
	}
	if (!(raw = fopen(s_tab.open->raw, "w+"))) {
		ERR("fopen '%s' '%s':", uri, s_tab.open->raw);
	}
	if (!(fmt = fopen(s_tab.open->fmt, "w"))) {
		ERR("fopen '%s' '%s':", uri, s_tab.open->fmt);
	}
	protocol = uri_protocol(uri);
	port = uri_port(uri);
	if (!port) port = protocol;
	if (!port) port = PROTOCOL;
	if (!protocol) protocol = port;
	if (save) {
		past_set(s_tab.open->past, uri);
	}
	s_tab.open->protocol = protocol;
	switch (protocol) {
	case URI_GOPHER: res = gph_req(raw, fmt, uri, new); break;
	case URI_GEMINI: res = gmi_req(raw, fmt, uri, new); break;
	case URI_FILE:   res = fsp_req(raw, fmt, uri); break;
	case URI_FTP:
	case URI_SSH:
	case URI_FINGER:
	case URI_HTTP:
	case URI_HTTPS:
	case URI_NUL:
	default:
		WARN("Unsupported protocol %d %s", protocol,
		     uri_protocol_str(protocol));
		res = NET_ERR;
	}
	if (fclose(raw) == EOF) {
		ERR("fclose '%s' '%s':", uri, s_tab.open->raw);
	}
	if (fclose(fmt) == EOF) {
		ERR("fclose '%s' '%s':", uri, s_tab.open->fmt);
	}
	switch (res) {
	case NET_NUL: break;    // Nothing to do
	case NET_ERR: return 0;
	case NET_RAW: cmd_run(s_pager, s_tab.open->raw); break;
	case NET_FMT: cmd_run(s_pager, s_tab.open->fmt); break;
	case NET_BIN: break;    // TODO(irek): Hmm what to do for binary files?
	case NET_URI: return onuri(new, 1);
	}
	return 1;
}

//
static void
onprompt(size_t sz, char *buf)
{
	// TODO(irek): I don't like passing the SZ and the creating
	// LAST buffer with size that is potentially different.  This
	// whole approach is fundamentally wrong.  I should use const
	// value for both.
	static char last[4096] = {0};
	char *arg, *uri;
	int i;
	switch (nav_cmd(buf, &arg)) {
	case CMD_QUIT:
		while (s_tab.n) {
			tab_close(&s_tab, 0);
		}
		exit(0);
		break;
	case CMD_HELP:
		printf(s_help);
		break;
	case CMD_SH_RAW:
		cmd_run(arg, s_tab.open->raw);
		break;
	case CMD_SH_FMT:
		cmd_run(arg, s_tab.open->fmt);
		break;
	case CMD_REPEAT:
		if (*last) {
			strcpy(buf, last);
			onprompt(sz, buf);
		}
		return; // Return to avoid defining CMD_REPEAT as last cmd
	case CMD_URI:
		onuri(buf, 1);
		break;
	case CMD_LINK:
		uri = onlink(atoi(buf));
		onuri(uri, 1);
		break;
	case CMD_PAGE_GET:
		onuri(s_tab.open->past->uri, 0);
		break;
	case CMD_PAGE_RAW:
		cmd_run(s_pager, s_tab.open->raw);
		break;
	case CMD_PAGE_URI:
		printf("%s\n", s_tab.open->past->uri);
		break;
	case CMD_TAB_GOTO:
		if (arg && (i = atoi(arg))) {
			tab_goto(&s_tab, i-1);
		} else {
			tab_print(&s_tab);
		}
		break;
	case CMD_TAB_ADD:
		tab_open(&s_tab);
		break;
	case CMD_TAB_PREV:
		tab_goto(&s_tab, s_tab.i -1);
		break;
	case CMD_TAB_NEXT:
		tab_goto(&s_tab, s_tab.i +1);
		break;
	case CMD_TAB_OPEN:
		// TODO(irek): Tab duplication should also copy
		// history from current tab.
		if (arg) {
			uri = (i = atoi(arg)) ? onlink(i) : arg;
		} else {
			uri = s_tab.open->past->uri;
		}
		tab_open(&s_tab);
		onuri(uri, 1);
		break;
	case CMD_TAB_CLOSE:
		if (s_tab.n <= 1) {
			printf("Can't close last tab\n");
			break;
		}
		tab_close(&s_tab, arg ? atoi(arg)-1 : s_tab.i);
		break;
	case CMD_HIS_LIST:
		// TODO(irek): It might be possible that when I
		// implement the file:// protocol then I could have
		// global history list (for all tabs from current
		// session and the past) as a file in one of protocols
		// like GOPHER.  Then openine history list will be
		// just an opening a file and serving it as regular
		// page in current tab.
		WARN("Not implemented");
		break;
	case CMD_HIS_PREV:
		uri = past_get(s_tab.open->past, -1);
		onuri(uri, 0);
		break;
	case CMD_HIS_NEXT:
		uri = past_get(s_tab.open->past, +1);
		onuri(uri, 0);
		break;
	case CMD_GET_RAW:
		copy(s_tab.open->raw, arg);
		break;
	case CMD_GET_FMT:
		copy(s_tab.open->fmt, arg);
		break;
	case CMD_CANCEL:
	case CMD_NUL:
		return;
	default:
		ERR("Unreachable");
	}
	strcpy(last, buf);
}

int
main(int argc, char *argv[])
{
	char *env, buf[4096];
	int i;
	ARGBEGIN {
	case 'v':
		printf(VERSION "\n");
		return 0;
	case 'h':
		usage();
		return 0;
	default:
		usage();
		return 1;
	} ARGEND
	s_pager = (env = getenv("PAGER")) ? env : "less -XI";
	for (i = 0; i < argc; i++) {
		tab_open(&s_tab);
		onuri(argv[i], 1);
	}
	if (!s_tab.n) {
		tab_open(&s_tab);
	}
	while (1) { // Prompt
		printf("yupa(%d/%d)> ", s_tab.i+1, s_tab.n);
		if (!fgets(buf, sizeof(buf), stdin)) {
			WARN("fgets:");
			continue;
		}
		buf[strlen(buf)-1] = 0;
		onprompt(sizeof(buf), buf);
	}
	return 0;
}
