/**
 * https://www.cnblogs.com/straight/articles/7660889.html
 */
#include <assert.h>
#include <sys/sysinfo.h>
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


#include "srp.h"
#include "net.h"

/* global objects */
epoll_t g_ep[MAX_CORES];



static int get_cpu_core()
{
	int count = sysconf(_SC_NPROCESSORS_ONLN);
	if (count > MAX_CORES)
	{
		LOG("Warning : actual core count(%d) is larger than expected(%d).\n", count, MAX_CORES);
	}
	return count;
}


/* Run epoll on relevant CPU core */
void * start_listen(void * arg)
{
#if 0	
	int fd, new_fd, struct_len, numbyts, core = *(int *)arg, i;
	char buf[BUF_SIZE] = {0};
	
	struct sockaddr_in server_addr, client_addr[SOCK_POOL_SIZE];

	/* Set socket info */
	server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    struct_len = sizeof(struct sockaddr_in);
#endif
	/* set epoll's id */
	int core = *(int *)arg;
	g_ep[core] = new_epoll(NULL, PORT);
	g_ep.start();
		
	return NULL;
}

int main(int argc, char * const argv[])
{
	int i, thread_count = get_cpu_core();
	pthread_t pthread[MAX_CORES];
	cpu_set_t cpuset;
		
	if (argc > 0)
	{
		thread_count = atoi(argv[0]);
	}
	
	thread_count = thread_count > MAX_CORES ? MAX_CORES : thread_count;
	LOG("Thread count reset to %d.\n", thread_count);
	
	for (i=0; i<thread_count; i++)
	{
		/* Create child thread */
		if (pthread_create(&pthread[i], NULL, start_listen, (void *)&i))
		{
			LOGE("Error : thread %d create failed.\n", i);
			return FAIL;
		}
		/* Bind to CPU core */
		CPU_ZERO(&cpu_set);
		CPU_SET(i, &cpu_set);
		assert(!pthread_set_affinity_np(pthread[i], sizeof(cpu_set_t), &cpu_set));
	}
	
	for (i=0; i<thread_count; i++)
	{
		if (pthread_join(pthread[i], NULL))
		{
			LOGE("Error : thread %d wait failed.\n", i);
			return FAIL;
		}
	}
	
	return OK;
}