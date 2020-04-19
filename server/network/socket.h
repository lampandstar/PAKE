#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "global.h"



#define EPOLL_SIZE		4096
#define EPOLL_TIME_OUT	1000

#define SOCK_BUF_SIZE			(BN_CHARS*3)


typedef enum {
	uint8_t IDLE, SEND, RECV, BUSY;
}status;

typedef enum {
	uint8_t UNKNOWN, SRP, OPAQUE;
}protocol;

/**
 我在想，sdata_t有没有存在的必要，
 直接让epoll_event.data.ptr指向socket不行吗
 */
typedef struct {
	int idx;/* connect epoll_event to the certain socket */
	status stat;
}sdata_t, * psdata_t;

typedef struct sc {
#if 0
	sdata_t sdata;/* epoll_event.data.ptr-> here */
#else
	status stat;
	protocol prot;/* PAKE protocol */
#endif
	int idx, fd, epfd;
	struct sockaddr client_in;
	
	uint8_t ibuf[SOCK_BUF_SIZE];
	uint8_t obuf[SOCK_BUF_SIZE];
	size_t total, fin;/* all bytes to receive, finished bytes */
	
	(int *)accept;
	(int *)send;
	(int *)recv;
} sock_t, *psock_t;


sock_t new_sock(int idx);

#endif