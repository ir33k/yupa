#include <assert.h>
#include <netdb.h>
#include <string.h>
#include "le.h"
#include "net.h"

int
tcp(char *host, int port)
{
	int i, sfd;
	struct hostent *he;
	struct sockaddr_in addr;
	assert(host);
	assert(port > 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if ((sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		WARN("socket %s %d:", host, port);
		return 0;
	}
	if ((he = gethostbyname(host)) == 0) {
		WARN("gethostbyname %s %d:", host, port);
		return 0;
	}
	for (i=0; he->h_addr_list[i]; i++) {
		memcpy(&addr.sin_addr.s_addr, he->h_addr_list[i], sizeof(in_addr_t));
		if (connect(sfd, (struct sockaddr*)&addr, sizeof(addr))) {
			continue;
		}
		return sfd;	// Success
	}
	WARN("failed to connect with %s %d:", host, port);
	return 0;
}

int
req(char *host, int port, char *path)
{
	int sfd;
	assert(host);
	assert(port > 0);
	if ((sfd = tcp(host, port)) < 0) {
		return 0;
	}
	if (path && send(sfd, path, strlen(path), 0) == -1) {
		WARN("send %s %d %s:", host, port, path);
		return 0;
	}
	if (send(sfd, "\r\n", 2, 0) == -1) {
		WARN("send %s %d %s:", host, port, path);
		return 0;
	}
	return sfd;
}
