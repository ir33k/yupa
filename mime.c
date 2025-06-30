#include <string.h>
#include <strings.h>
#include "util.h"
#include "mime.h"

enum mime
mime_path(char *path)
{
	char *extension;

	if (!path || !path[0])
		return 0;

	extension = strrchr(path, '.');
	if (!extension) return BINARY;
	if (strcasecmp(extension, ".txt"))  return TEXT;
	if (strcasecmp(extension, ".md"))   return TEXT;
	if (strcasecmp(extension, ".gph"))  return GPH;
	if (strcasecmp(extension, ".gmi"))  return GMI;
	if (strcasecmp(extension, ".html")) return HTML;
	if (strcasecmp(extension, ".jpg"))  return IMAGE;
	if (strcasecmp(extension, ".jpeg")) return IMAGE;
	if (strcasecmp(extension, ".png"))  return IMAGE;
	if (strcasecmp(extension, ".bmp"))  return IMAGE;
	if (strcasecmp(extension, ".gif"))  return IMAGE;
	if (strcasecmp(extension, ".mp4"))  return VIDEO;
	if (strcasecmp(extension, ".mov"))  return VIDEO;
	if (strcasecmp(extension, ".avi"))  return VIDEO;
	if (strcasecmp(extension, ".mkv"))  return VIDEO;
	if (strcasecmp(extension, ".wav"))  return AUDIO;
	if (strcasecmp(extension, ".mp3"))  return AUDIO;
	if (strcasecmp(extension, ".pdf"))  return PDF;

	return 0;
}

enum mime
mime_header(char *str)
{
	if (!str || !str[0])
		return 0;

	if (startswith(str, "text/gemini"))     return GMI;
	if (startswith(str, "text/html"))       return HTML;
	if (startswith(str, "text/gopher"))     return GPH;
	if (startswith(str, "text/"))           return TEXT;
	if (startswith(str, "image/"))          return IMAGE;
	if (startswith(str, "video/"))          return VIDEO;
	if (startswith(str, "audio/"))          return AUDIO;
	if (startswith(str, "application/pdf")) return PDF;
	if (startswith(str, "application/octet-stream")) return BINARY;

	return 0;
}
