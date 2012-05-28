// client.c
// 1628872598@qq.com

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#define PORTNO "12345"

int main(int argc, char *argv[])
{
	struct addrinfo hints, *result, *rp;
	int fd;
	int rt;
	int errsv = 0;
	char buf[128] = {'\0'};
	time_t tm;
	char hostnm[20] = {'\0'};

	if (argc < 2){
		printf("%s <host>\n", argv[0]);
		return -1;
	}
	if (gethostname(hostnm, sizeof hostnm - 1) == -1){
		perror("gethostname");
		return -1;
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

	rt = getaddrinfo(argv[1], PORTNO, &hints, &result);
	errsv = errno;
	if (rt){
		printf("%s: %s\n", "getaddrinfo gai_strerror", gai_strerror(rt));
		if (errsv)
			perror("getaddrinfo");
		return rt;
	}
	for (rp = result; rp; rp = rp->ai_next){
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (fd == -1)
			continue;
		if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;
		close(fd);
	}
	if (!rp){
		printf("Could not connect socket to any address\n");
		return !rp;
	}
	freeaddrinfo(result);

	for (;;){

		sleep(1); //Block revc / send used - no MSG_DONTWAIT
			  //temporary for demo only
			  //Normal code does not require this sleep.

		// send
		time(&tm);
		snprintf(buf, sizeof buf, "%s %s", hostnm, ctime(&tm));
		rt = send(fd, buf, strlen(buf), 0); //MSG_DONTWAIT
		errsv = errno;
		if (rt == -1){
			printf("%s: %s\n", "send", strerror(errsv));
			if (errsv == ECONNRESET || errsv == ENOTCONN || errsv == EPIPE){
				close(fd);
				break;
			}
		}

		// recv
		memset(buf, 0, sizeof buf);
		rt = recv(fd, buf, sizeof buf - 1, 0); //MSG_DONTWAIT
		errsv = errno;
		if (rt == -1){
			printf("%s: %s\n", "recv", strerror(errsv));
			if (errsv == ECONNRESET || errsv == ENOTCONN){
				close(fd);
				break;
			}
		}
		if (rt == 0){
			close(fd);
			printf("%s: %s\n", "recv", "Peer has performed an orderly shutdown");
			break;
		}
		if (strlen(buf))
			printf("recv: server time: %d %s", strlen(buf), buf);
	}
	close(fd);
	return 0;
}

