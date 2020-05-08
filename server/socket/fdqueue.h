#include <stdio.h>
#include "global.h"

#define INITIAL_SIZE		1024
#define MALLOC_UNIT_SIZE	256

typedef struct _queue {
	int head, tail;
	size_t arr_size, size;
	
	/* memory area */
	FD * arr;/* 考虑换成union */
	
	/* methods */
	(FD *)pop;
	(int *)push;
	(void *)clear;
	(void *)destroy;
}fd_queue;

BOOL fdq_is_empty(OUT fd_queue * fdq);
FD fdq_pop(OUT fd_queue * fdq);
int fdq_push(OUT fd_queue * fq, FD e);
void dfq_clear(OUT fd_queue * fq);
void dfq_destroy(OUT fd_queue * fq);

/* constructor */
fd_queue fd_queue_init()
{
	fd_queue fdq;
	
	fdq.head = fdq.ail = 0;
	if ((fdq.arr = calloc(INITIAL_SIZE, sizeof(FD))) == NULL)
	{
		return FAIL;
	}
	fdq.arr_size = INITIAL_SIZE;
	
	fdq.sis_empty = fdq_is_empty;
	fdq.spop = fd_pop;
	fdq.spush = fd_push;
	fdq.sclear = fd_clear;
	fdq.sdestroy = fd_destroy;
	
	return fdq;
}


/* private method */
static int adjust(OUT fd_queue * fdq)
{
	size_t size = fdq->arr_size + MALLOC_UNIT_SIZE;
	FD * p = fdq->arr;
	
	if ((fdq->arr = realloc(fdq->arr, size * sizeof(FD)) == NULL)
	{
		fdq->arr = p;
		return FAIL;
	}
	
	fdq->arr_size = size;
	return OK;
}

BOOL fdq_is_empty(OUT fd_queue * fdq)
{
	return fdq->size == 0 ? TRUE : FALSE;
}

/* public methods */
FD fdq_pop(OUT fd_queue * fdq)
{
	FD r = NULL;
	if (fdq->size == 0 || fdq->arr_size == 0)
	{
		LOG("The queue is empty.\n");
		return -1;
	}
	
	r = fdq->arr[fdq->head++];
	fdq->head %= fdq->arr_size;
	fdq->size--;
	return r;
}

int fdq_push(OUT fd_queue * fq, FD e)
{
	if (fdq->size+1 >= fdq->arr_size)
	{
		if (adjust(fq) != OK)
		{
			LOGE("Failed to alloc memory.\n");
			return ERR;
		}
	}
	
	fdq->arr[fdq->tail++] = e;
	fdq->tail %= fdq->arr_size;
	
	return OK;
}

void dfq_clear(OUT fd_queue * fq)
{
	fq->head = fq->tail = fq->size = 0;
}

void dfq_destroy(OUT fd_queue * fq)
{
	dfq_clear(fq);
	free(fq->arr);
	fq->arr = NULL;
	fq->arr_size = 0;
}