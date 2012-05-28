// server.c
// 1628872598@qq.com

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <stdlib.h>

#define PORTNO "12345"
#define BACKLOG 5
#define MAXEVENTS 10

int setnonblocking(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1){
		perror ("fcntl");
		return -1;
	}
	flags |= O_NONBLOCK;
	flags = fcntl(fd, F_SETFL, flags);
	if (flags == -1){
		perror ("fcntl");
		return -1;
	}
	return 0;
}

int main(void)
{
	struct addrinfo hints, *result, *rp;
	struct sockaddr addr;
	socklen_t addrlen;
	int sfd, ifd;
	int optval;
	int rt, errsv = 0;
	char host[NI_MAXHOST], service[NI_MAXSERV];
	char buf[128] = {'\0'};
	struct epoll_event ev, *evp;
	int efd, iefd, nefd;
	char hostnm[20] = {'\0'};
	time_t tm;

	if (gethostname(hostnm, sizeof hostnm - 1) == -1){
		perror("gethostname");
		return -1;
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

	rt = getaddrinfo(NULL, PORTNO, &hints, &result);
	errsv = errno;
	if (rt){
		printf("%s: %s\n", "getaddrinfo gai_strerror", gai_strerror(rt));
		if (errsv)
			perror("getaddrinfo");
		return rt;
	}

	optval = 1;
	for (rp = result; rp; rp = rp->ai_next){
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;
		if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1){
			perror("setsockopt");
			return -1;
		}
		if (!bind(sfd, rp->ai_addr, rp->ai_addrlen))
			break;
		close(sfd);
	}
	if (!rp){
		printf("Could not bind socket to any address\n");
		return !rp;
	}

	if (listen(sfd, BACKLOG) == -1){
		perror("listen");
		return -1;
	}
	freeaddrinfo(result);

	efd = epoll_create1(0);
	if (efd == -1){
		perror("epoll_create");
		return -1;
	}
	ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
	ev.data.fd = sfd;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev) == -1){
		perror("epoll_ctl");
		return -1;
	}
	errsv = 0;
	evp = calloc(MAXEVENTS, sizeof ev);
	errsv = errno;
	if (!evp){
		printf("%s: %s\n", "calloc failed ", errsv ? strerror(errsv) : "");
		return !evp;
	}

	for (;;){
		nefd = epoll_wait(efd, evp, MAXEVENTS, -1);
		if (nefd == -1){
			perror("epoll_wait");
			return -1;
		}

		for (iefd = 0; iefd != nefd; iefd++){
			if (evp[iefd].data.fd == sfd) {
				addrlen = sizeof addr;
				ifd = accept(sfd, &addr, &addrlen);
				if (ifd == -1){
					perror("accept");
					return -1;
				}
				if (!getnameinfo(&addr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0))
					printf("Connection from %s %s\n", host, service);
				else
					printf("Unknown %s %s\n", host, service);
				if (setnonblocking(ifd)){
					printf("setnonblocking failed\n");
					return -1;
				}
				ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
				ev.data.fd = ifd;
				if (epoll_ctl(efd, EPOLL_CTL_ADD, ifd, &ev) == -1){
					perror("epoll_ctl");
					return -1;
				}
			} else {
				if ((evp[iefd].events & EPOLLIN) == EPOLLIN){
					memset(buf, 0, sizeof buf);

					//partial read or recv? not handled
					rt = recv(evp[iefd].data.fd, buf, sizeof buf - 1, 0); //MSG_DONTWAIT
					errsv = errno;
					if (rt == -1){
						printf("%s: %s\n", "recv", strerror(errsv));
						if (errsv == ECONNRESET || errsv == ENOTCONN){
							close(evp[iefd].data.fd);
							continue;
						}
					}
					if (rt == 0){
						printf("%s: %s\n", "recv", "Peer has performed an orderly shutdown");
						close(evp[iefd].data.fd);
						continue;
					}
					if (strlen(buf))
						printf("recv: client time: %d %s", strlen(buf), buf);
				}

				if ((evp[iefd].events & EPOLLOUT) == EPOLLOUT){
					time(&tm);
					snprintf(buf, sizeof buf, "%s %s", hostnm, ctime(&tm));

					//partial write or send? not handled
					rt = send(evp[iefd].data.fd, buf, strlen(buf), 0); //MSG_DONTWAIT
					errsv = errno;
					if (rt == -1){
						printf("%s: %s\n", "send", strerror(errsv));
						if (errsv == ECONNRESET || errsv == ENOTCONN || errsv == EPIPE || errsv == EBADF){
							close(evp[iefd].data.fd);
							continue;
						}
					}
				}

				if ((evp[iefd].events & EPOLLERR) == EPOLLERR ||
					(evp[iefd].events & EPOLLHUP) == EPOLLHUP ||
					((evp[iefd].events & EPOLLIN) != EPOLLIN && (evp[iefd].events & EPOLLOUT) != EPOLLOUT))
				{
					printf ("Unknown event: %d\n", evp[iefd].events);
					close (evp[iefd].data.fd);
					continue;
				}
			}
		}
	}

	// not reachable
	free(evp);
	close(sfd);
	return 0;
}



