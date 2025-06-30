/* MIME file type */

enum mime {
	REDIRECT=-2,
	SEARCH=-1,
	BINARY=0,
	TEXT,
	GPH,
	GMI,
	HTML,
	IMAGE,
	VIDEO,
	AUDIO,
	PDF
};

enum mime mime_path(char *path);
enum mime mime_header(char *str);
