#include <stdio.h>
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
	if (!extension) return MIME_TEXT;
	if (!strcasecmp(extension, ".txt"))  return MIME_TEXT;
	if (!strcasecmp(extension, ".md"))   return MIME_TEXT;
	if (!strcasecmp(extension, ".gph"))  return MIME_GPH;
	if (!strcasecmp(extension, ".gmi"))  return MIME_GMI;
	if (!strcasecmp(extension, ".html")) return MIME_HTML;
	if (!strcasecmp(extension, ".jpg"))  return MIME_IMAGE;
	if (!strcasecmp(extension, ".jpeg")) return MIME_IMAGE;
	if (!strcasecmp(extension, ".png"))  return MIME_IMAGE;
	if (!strcasecmp(extension, ".bmp"))  return MIME_IMAGE;
	if (!strcasecmp(extension, ".gif"))  return MIME_IMAGE;
	if (!strcasecmp(extension, ".mp4"))  return MIME_VIDEO;
	if (!strcasecmp(extension, ".mov"))  return MIME_VIDEO;
	if (!strcasecmp(extension, ".avi"))  return MIME_VIDEO;
	if (!strcasecmp(extension, ".mkv"))  return MIME_VIDEO;
	if (!strcasecmp(extension, ".wav"))  return MIME_AUDIO;
	if (!strcasecmp(extension, ".mp3"))  return MIME_AUDIO;
	if (!strcasecmp(extension, ".pdf"))  return MIME_PDF;

	return 0;
}

enum mime
mime_header(char *str)
{
	if (!str || !str[0])
		return 0;

	if (startswith(str, "text/gemini"))              return MIME_GMI;
	if (startswith(str, "text/html"))                return MIME_HTML;
	if (startswith(str, "text/"))                    return MIME_TEXT;
	if (startswith(str, "image/"))                   return MIME_IMAGE;
	if (startswith(str, "video/"))                   return MIME_VIDEO;
	if (startswith(str, "audio/"))                   return MIME_AUDIO;
	if (startswith(str, "application/pdf"))          return MIME_PDF;
	if (startswith(str, "application/octet-stream")) return MIME_BINARY;
	if (startswith(str, "application/gopher-menu"))  return MIME_GPH;

	return 0;
}
