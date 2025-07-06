/* MIME file type */

enum mime {BINARY=0, TEXT, GPH, GMI, HTML, IMAGE, VIDEO, AUDIO, PDF};

enum mime mime_path(char *path);
enum mime mime_header(char *str);
