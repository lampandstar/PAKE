#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "socket.h"
#include "fdqueue.h"

#define EPOLL_SIZE		4096
#define EPOLL_TIME_OUT	1000

typedef sock_t sock_pool[SOCK_POOL_SIZE];

typedef struct ep {
	int core_id, port;
	int id;/* return value of epoll_create, as well as ID of epoll object */
	int listener, fd;
	BOOL exit;
	
	//psock_t sock[MAX_CORES];
	sock_pool * sock;/* dynamic dim * SOCK_POOL_SIZE */
	size_t sock_size;
	
	fd_queue idle_socks;
	
	/*(int *)init(struct ep *, char *, int);/* port */
	(int *)start;//(struct ep *);
	(int *)stop;//(struct ep *);
	(int *)destroy;//(struct ep *);
} epoll_t, *pepoll_t;

epoll_t new_epoll(IN char * ip, IN int port);

#endif