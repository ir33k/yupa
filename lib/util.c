#include <assert.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "util.h"
#include "le.h"

char *
home(void)
{
	struct passwd *pw = getpwuid(getuid());
	return pw ? pw->pw_dir : "";
}

char *
_join(int _ignore, ...)
{
	static char tmp[BUFSIZ];
	va_list ap;
	size_t sum=0, len;
	char *arg;
	va_start(ap, _ignore);
	while ((arg = va_arg(ap, char *))) {
		len = strlen(arg);
		// Nothing more can fit the TMP but don't error out.
		// This function doesn't try to be always correct
		// rather it tires to by convenient.
		if (sum + len + 1 > sizeof(tmp)) {
			WARN("BSIZ %d exceeded with %s", sizeof(tmp), arg);
			break;
		}
		memcpy(tmp + sum, arg, len + 1);
		sum += len;
	}
	va_end(ap);
	return tmp;
}

char *
strrand(size_t len)
{
	static const char *allow =
		"ABCDEFGHIJKLMNOPRSTUWXYZ"
		"abcdefghijklmnoprstuwxyz"
		"0123456789";
	static const size_t siz = sizeof(allow);
	static int seed=0;
	static char str[32];
	assert(len < sizeof(str));
	srand(time(0) + seed++);
	for (str[len]=0; len--;) {
		str[len] = allow[rand() % siz];
	}
	return str;
}

void
tmpf(char *prefix, char dst[FILENAME_MAX])
{
	int fd;
	assert(prefix);
	assert(dst);
	do {
		sprintf(dst, "%s%s-%s", "/tmp/", prefix, strrand(6));
	} while (!access(dst, F_OK));
	if ((fd = open(dst, O_RDWR | O_CREAT)) == -1) {
		ERR("open %s:", dst);
	}
	if (close(fd)) {
		ERR("close %s:", dst);
	}
}

void
cmd_run(char *cmd, char *filename)
{
	char buf[FILENAME_MAX+1024];
	assert(cmd);
	assert(filename);
	snprintf(buf, sizeof(buf), "%s %s", cmd, filename);
	system(buf);
}

void
copy(char *src, char *dst)
{
	FILE *fd[2];
	char buf[4096], *tmp;
	size_t siz;
	assert(src && src[0]);
	if (!dst) {
		printf("Missing destination file path.\n");
		return;
	}
	tmp = JOIN(dst[0] == '~' ? home() : "",
		   dst[0] == '~' ? dst +1 : dst);
	if (!(fd[0] = fopen(src, "rb"))) {
		WARN("%s %s fopen(src):", src, tmp);
		return;
	}
	if (!(fd[1] = fopen(tmp, "wb"))) {
		WARN("%s %s fopen(dst):", src, tmp);
		goto clean;
	}
	while ((siz = fread(buf, 1, sizeof(buf), fd[0]))) {
		if (fwrite(buf, 1, siz, fd[1]) != siz) {
			WARN("%s %s fwrite:", src, tmp);
		}
	}
	if (fclose(fd[1])) {
		WARN("%s %s fclose(dst):", src, tmp);
	}
clean:	if (fclose(fd[0])) {
		WARN("%s %s fclose(src):", src, tmp);
	}
}
