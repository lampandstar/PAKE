/**
 * socket : https://www.cnblogs.com/straight/articles/7660889.html
 * OOP with C : https://www.kristijorgji.com/blog/simulating-object-oriented-programming-oop-in-c/
 */

#ifndef __NET_H__
#define __NET_H__

#include "global.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "socket.h"

/* public methods */
int sock_accept(psock_t sock, int client, struct sockaddr addr, socklen_t len);

int sock_send(psock_t sock);

int sock_recv(psock_t sock);

sock_t new_sock(int idx)
{
	sock_t sock;
	sock.idx = idx;
	memset(sock.buf, 0, sizeof(sock.buf));
	sock.fin = sock.total = 0;
#if 0
	sock.sdata = {idx, IDLE};
#else
	sock.stat = IDLE;
	sock.prot = UNKNOWN;
#endif
	/* public methods */
	sock.accept = sock_accept;
	sock.send = sock_send;
	sock.recv = sock_recv;
	return sock;
}


/* private methods */
static int init_socket(int fd);
static int process_recv(psock_t sock, int len);


/* public */
/**
 * accept a connection from listener
 */
int sock_accept(psock_t sock, int client, struct sockaddr addr, socklen_t len)
{
	struct epoll_event evt;

	if (sock->stat != IDLE)
	{
		LOGE("Error : socket is busy.\n");
		return ERR;
	}
	
	sock->stat = RECV;
	sock->fd = client;/* from listenning socket */
	
	/* record addr of the request arrived */
	memcpy(&sock->client_in, addr, len);
	
	assert(sock->stat == IDLE);
	//m_inactive_count = 0;//?
	//m_state = State_Busy;
	assert(sock->fd == 0);

	if (init_socket(sock->fd) != OK)
	{
		LOGE("Error : setsockopt failed.\n");
		return ERR;/* or handle_err() */
	}

	evt.data.ptr = sock;
	evt.evt.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
	
	/* join to epoll again*/
	if (epoll_ctl(sock->epfd, EPOLL_CTL_ADD, sock->fd, &evt) < 0)
	{
		LOGE("Error : epoll_ctl error, code %d.\n", errno);
		return ERR;
	}
	
	return OK;
}

int sock_send(psock_t sock)
{
	int ret;
	struct epoll_event evt;
	
	do
	{
		/* data shall be prepared before */
		ret = send(sock->fd, sock->obuf + sock->fin, sock->total - sock->fin);
		
		if (ret < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				evt.data.ptr = sock;
				evt.events = EPOLLOUT | EPOLLONESHOT | EPOLLET;//why oneshot?
			
				if (epoll_ctl(sock->epfd, EPOLL_CTL_MOD, sock->fd, &evt) < 0)
				{
					LOGE("Error : add event to epoll failed, code: %d.\n", errno);
					return ERR;
				}
			}
			else
			{
				LOGE("Error : send returned %d, code: %d.\n", ret, errno);
				return ERR;
			}
		}
		else
		{
			sock->fin += ret;
		}
	} while (sock->fin < sock->total);
	
	if (sock->fin > sock->total)
	{
		LOGE("Error : buffer overflow, %dB to be sent while %dB actually sent.\n", sock->total, sock->fin);
		return ERR;
	}
	
	/* join into epoll again */
	sock.stat = IDLE;
	evt.data.ptr = sock;
	evt.events = EPOLLIN | EPOLL_CTL_MOD | EPOLLET;
	
	if (epoll_ctl(sock->epfd, EPOLL_CTL_MOD, sock->fd, &evt) < 0)
	{
		LOGE("Error : add event to epoll failed, code: %d.\n", errno);
		return ERR;
	}
	
	return OK;
}

int sock_recv(psock_t sock)
{
	int ret;
	struct epoll_event evt;
	
	if (sock->stat == SEND)
	{
		LOGE("Error : try to receive data while sending.\n");
		return ERR;
	}

	/*
	 Ethernet limits the length of message to 40~1500B
	 int recv( SOCKET s, char FAR *buf, int len, int flags);
	 
	*/
	ret = recv(sock->fd, sock->ibuf+sock->fin, SOCK_BUF_SIZE-sock->fin, 0);

	if (ret < 0)
	{
		/* resource temperarily unavailable, try again */
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			evt.data.ptr = sock;
			evt.events = EPOLLIN | EPOLLONESHOT | EPOLLET;//why oneshot?
			
			if (epoll_ctl(sock->epfd, EPOLL_CTL_MOD, sock->fd, &evt) < 0)
			{
				LOGE("Error : add event to epoll failed, code: %d.\n", errno);
				return ERR;
			}
		}
		else
		{
			LOGE("Error : recv returned %d, code: %d.\n", ret, errno);
			return ERR;
		}
	}
	else if (ret == 0)
	{
		LOGE("Error : recv returned %d.\n", ret);
		return ERR;
	}
	else
	{
		/* data correctly received */
		process_recv(sock, ret);
	}

	return OK;
}

/* private */
static int init_socket(int fd)
{
	int opts = 1;
	/* set socket to non-block */
	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) == -1)
	{
		LOGE("Error : failed to set socket to non-block mode, code: %d.\n", errno);
		return ERR;
	}
	/* disable Nagle */
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&opts, sizeof(int)) < 0)
	{
		LOGE("Error : setsockopt TCP_NODELAY failed, code: %d.\n", errno);
		return ERR;
	}

	/* set buffer size */
	opts = 1024 * 1024;
	
	if (setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char *)&opts, sizeof(int)) < 0)
	{
		LOGE("Error : setsockopt SO_RCVBUF failed, code: %d.\n", errno);
		return ERR;
	}

	if (setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char *)&opts, sizeof(int)) < 0)
	{
		LOGE("Error : setsockopt SO_SNDBUF failed, code: %d.\n", errno);
		return ERR;
	}
	return OK;
}

static int process_recv(psock_t sock, int len)
{
	uint8_t * b = sock->ibuf;
	sock->fin += len;
	LOGD("%d bytes received.\n", len);
	
	/**		General Protocol
	 * buffer = byts(4B LE) | content
	 * content = protocol NO.(1B) | data
	 */
	/* extract byts */
	if (sock->total == 0)
	{
		LOGD("Getting total bytes.\n");
		if (sock->fin > 3)
			sock->total = GETU32(b) - 5;
		else
			sock->recv(sock);
	}
	
	if (sock->prot == UNKNOWN)
	{
		LOGD("Getting PAKE protocol.\n");
		if (sock->fin > 4)
			sock->prot = b[3];
		else
			sock->recv(sock);
	}
	
	if (sock->fin < sock->total)
	{
		sock->recv(sock);
	}
	if (sock->fin == sock->total)
	{
		/* Put following procedures into srp.c? */
		b += 5;/* jump over length and protocol byte */
		switch (sock->proto)
		{
			/**
			 * PAKE protocol related issues
			 * launch to GPU or CPU according to overhead
			 */
			case SRP:
				/**
				 * deliver b to SRP program
				 */
				 
				/* AVX in CPU */
				sock->total = SOCK_BUF_SIZE;
				sock->fin = 0;
				if (compute(sock->obuf, &(sock->total), b, sock->total-5) != SRP_OK)
				{
					LOGE("Error : computation failed.\n");
					return ERR;
				}
				return sock.send();
				/**
				 * 问题：高负载时如何处理？
				 * 低负载时正常调用CPU程序，高负载时，如何积攒数据？
				 * 拷贝到全局大数组？貌似不归这一层管
				 */
				
				break;
			default:
				LOGE("Error : unknown or unsupported PAKE protocol (NO. %d).\n", sock->proto);
				return ERR;
		}
		/* Set socket to SEND or IDLE, then join into epoll */
		//sock->stat = IDLE/BUSY/SEND
		//sock->send(sock);
		//sock->fin = sock->total = 0;
	}
	else
	{
		LOGE("Error : buffer(%d B) exceeded total length(%d B).\n", sock->fin, sock->total);
	}
	
	return OK;
}