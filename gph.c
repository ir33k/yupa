/* Gopher protocol. */

#define GPH_PORT	70	/* Defaul port */

#if 0
/**/
static char *
gph_uri(char *file, int index)
{
	FILE *fp;
	static char uri[BUFSIZ];
	char c, buf[BUFSIZ], path[BUFSIZ], host[BUFSIZ];
	int port;
	assert(file);
	assert(index > 0);
	if (!(fp = fopen(file, "r"))) {
		return 0;
	}
	while (fgets(buf, BUFSIZ, fp)) {
		if (buf[0] == '.') {
			break;
		}
		if (buf[0] == 'i') {
			continue;
		}
		if (--index) {
			continue;
		}
		/* TODO(irek): Here I need to add special case
		 * for non Gopher links. */
		/* Found */
		sscanf(buf, "%c%*[^\t]\t%[^\t]\t%[^\t]\t%d",
		       &c, path, host, &port);
		sprintf(uri, "gopher://%.1024s:%d/%c%.1024s",
			host, port, c, path);
		return uri;
	}
	fclose(fp);
	return 0;
}
#endif
