/**
 * socket : https://www.cnblogs.com/straight/articles/7660889.html
 * epoll : https://www.cnblogs.com/panfeng412/articles/2229095.html
 * OOP with C : https://www.kristijorgji.com/blog/simulating-object-oriented-programming-oop-in-c/
 */

#ifndef __NET_H__
#define __NET_H__

#include <sys/epoll.h>
#include "global.h"


#define SOCK_POOL_SIZE		1024
#define SOCK_POOL_EXPAND	1
#define BACKLOG_SIZE		10000/* max number of waiting connections */

#define IP
#define PORT 				8080

#define MAX_CORES			32
#define EVENT_NUM			16

#define BUF_SIZE			(BN_CHARS*3)



//////////////////以下内容挪到 epoll.* 和 socket.*/////////////////

/*******************************************************************************
 * SOCKET
 ******************************************************************************/
typedef enum {
	int IDLE, SEND, RECV;
}status;

typedef struct {
	int idx;/* connect epoll_event to the certain socket */
	status stat;
}sdata_t, * psdata_t;

typedef struct sc {
	/* epoll_event.data.ptr-> here */
	sdata_t sdata;
	int fd, epfd;
	
	(int *)accept;
	(int *)send;
	(int *)recv;
} sock_t, *psock_t;


/**
 * 函数实现放在socket.c中
 */
sock_t sock_init(int idx)
{
	sock_t sock;
	sock.idx = idx;
	sock.send = ;
	sock.recv = ;
	sock.sdata = {idx, IDLE};
	return sock;
}





/*******************************************************************************
 * EPOLL
 ******************************************************************************/

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
	(int *)start(struct ep *);
	(int *)stop(struct ep *);
} epoll_t, *pepoll_t;



////////////////////////// epoll.c ////////////////////////////////
epoll_t epoll_init(IN char * ip, IN int port)
{
	epoll_t ep;
	int i, j, idx;
	
	if (ip == NULL)
	{
		ep.addr = INADDR_ANY;
	}
	else
	{
		ep.addr = inet_addr(ip);
	}
	ep.exit = FALSE;
	ep.port = PORT;
	if ((ep.sock = calloc(SOCK_POOL_EXPAND, sizeof(sock_pool))) == NULL)
	{
		LOGE("Error : failed to create epoll.\n");
		return NULL;
	}
	ep.sock_size = SOCK_POOL_EXPAND;
	ep.idle_socks = fd_queue_init();
	
	for (i=0; i<ep.sock_size; i++)
	{
		idx = i*SOCK_POOL_SIZE;
		for (j=0; j<SOCK_POOL_SIZE; j++)
		{
			ep.sock[i][j] = sock_init(idx + j);
			ep.idle_socks.push(idx + j);
		}
	}
	
	/* assign methods to object */
	ep.start = epoll_start;
	ep.stop = epoll_stop;
	return ep;
}

/* private */
static int epoll_create_listener(pepoll_t ep)
{
	int listener;
	struct sockaddr_in saddr;
	struct epoll_event evt;
	const BOOL reusable = TRUE;
	
	/* Create listener socket */
	if ((ep->listener = socket(AF_INET, SOCK_STREAM, IPROTO_TCP)) < 0)
	{
		LOGE("Error : listener socket create failed.\n");
		return FAIL;
	}

	/* Set listener to non-block */
#ifdef MTCP_DPDK

#else
	if (fcntl(ep->listener, F_SETFL, fcntl(ep->listener, F_GETFL, 0) | O_NONBLOCK) == -1)
#endif
	{
		LOGE("Error : set non-block mode failed.\n");
		return FAIL;
	}
	
	/* Bind to port */
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = ep->addr;
	saddr.sin_port = htons(ep->port);
	
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEPORT, &reusable, sizeof(BOOL)) < 0)
	{
		LOGE("Error : set socket option failed.\n");
		return FAIL;
	}
	
	if (bind(listener, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in)) < 0)
	{
		LOGE("Error : bind port %d failed.\n", ep->port);
		return FAIL;
	}
	
	if (listen(listener, BACKLOG_SIZE) < 0)
	{
		LOGE("Error : add listenning failed.\n");
		return FAIL;
	}
	
	evt.events = EPOLLIN;
#ifdef MTCP_DPDK

#else
	evt.data.fd = listener;
#endif
	/* join listener to epoll */
	epoll_ctl(ep, EPOLL_CTL_ADD, listener, &evt);
	return listener;
}

/* private */
static int epoll_accept_listener(pepoll_t ep)
{
	int sclient, idx, i, j, base = ep->sock_size * SOCK_POOL_SIZE;
	struct sockaddr client_in;
	sock_pool p = ep->sock;
	socklen_t clen = sizeof(struct sockaddr_in);

	/* get connections from listener */
	if ((sclient = accept(ep->listener, &client_in, &clen)) > 0)
	{
		/* dispath sclient to an idle socket */
		idx = ep->idle_socks.pop();
		if (idx < 0)
		{
			LOGD("No available socket, try expanding socket pool.\n");
			/* expand socket pool */
			if (ep->sock = realloc(p, ep->sock_size+SOCK_POOL_EXPAND) == NULL)
			{
				ep->sock = p;
				LOGE("Expand socket pool failed.\n");
				close(sclient);
				return FAIL;
			}
			else
			{
				p += ep->sock_size;
				for (i=0; i<SOCK_POOL_EXPAND-1; i++)
				{
					/* p[i][0] = base + i*SOCK_POOL_SIZE */
					for (j=0; j<SOCK_POOL_SIZE; j++)
					{
						idx = base + i*SOCK_POOL_SIZE + j;
						p[i][j] = sock_init(idx);
						ep->idle_socks.push(idx);
					}
				}
				ep->sock_size += SOCK_POOL_EXPAND;
			}
			/* pick up an idle socket again */
			idx = ep->idle_socks.pop();
		}
		/* dispatch connection to sock[idx] */
		i = idx / SOCK_POOL_SIZE;
		j = idx % SOCK_POOL_SIZE;
		ep->sock[i][j].epfd = ep->fd;
		ep->sock[i][j].accept(sclient, &client_in, clen);
	}
	
	return sclient;
}

/* 子线程中？ */
static int epoll_start(pepoll_t ep)
{
	int i, j, n, nfds;
	struct epoll_event evt[EVENT_NUM];
	psdata_t evt_data;
	psock_t sock;
	
	if ((ep->fd = epoll_create(EPOLL_SIZE)) < 0)
	{
		LOGE("Error : epoll create failed.\n");
		return FAIL;
	}
	
	/**
	 * int epoll_create(int size)
	 * Since Linux 2.6.8, the size argument is ignored, but must be greater than zero
	 */
	ep->listener = epoll_create_listener();

	while (TRUE)
	{
		if (ep->exit)
		{
			LOGD("Exit epoll %d.\n", ep->fd);
			break;
		}
		/* Events to handle */
		nfds = epoll_wait(ep->fd, evt, EVENT_NUM, EPOLL_TIME_OUT);
		
		if (nfds <= 0) continue;
		
		for (i=0; i<nfds; i++)
		{
			if (evt[i].data.fd != ep->listener)
			{
				/* data to r/w : get socket id from event.data.ptr */
				evt_data = (psdata_t)evt[i].data.ptr;
				n = evt_data->idx / SOCK_POOL_SIZE;
				j = evt_data->idx % SOCK_POOL_SIZE;
				sock = ep->sock[n][j];

				if (evt[i].events & EPOLLIN)
				{
					/* begin or continue recv */
					sock->recv();
				}
				else if (evt[i].events & EPOLLOUT)
				{
					/* begin or continue send */
					sock->send();
				}
				else
				{
					LOGE("Error : unsupported operation type (%u).\n", evt[i].events);
					return ERR;
				}
			}
			/* connections arrived listener, dispatch to an idle socket */
			else
			{	/* accept all connections from listener */
				while (epoll_accept_listener(ep) > 0);
			}
		}
	}
	return OK;
}

int epoll_destroy()
{
	
	return OK;
}
////////////////////////// epoll.c ////////////////////////////////




#endif