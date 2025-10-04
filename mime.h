/* MIME file type */

enum mime {
	MIME_NONE,
	MIME_BINARY,
	MIME_TEXT,
	MIME_GPH,
	MIME_GMI,
	MIME_HTML,
	MIME_IMAGE,
	MIME_VIDEO,
	MIME_AUDIO,
	MIME_PDF
};

enum mime mime_path(char *path);
enum mime mime_header(char *str);
