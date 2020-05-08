#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define IN const
#define OUT

#define API __attribute__((visibility ("default")))

#define OK		0
#define FAIL	-1
#define ERR		-2

#ifndef BOOL
#define BOOL char

#define TRUE	1
#define FALSE	0
#endif

#define FD		int/* file descriptor */


#define LOG		printf
#define LOGE	printf
#ifdef _DEBUG
 #define LOGD	printf
#else
 #define LOGD	
#endif

/**
 * some macros shared between CPU and GPU
 */
#include "params.h"

#define MASK52	0xFFFFFFFFFFFFFull

#define CMP_EQ			0
#define CMP_GT			1
#define CMP_LT			-1

#define MAX_BITS		MODULO_SIZE
#define MAX_BYTS		((MAX_BITS+7) >> 3)
#define MAX_WDS			((MAX_BYTS+1) >> 1)
#define MAX_BWDS		((MAX_WDS +1) >> 1)
#define MAX_QWDS		((MAX_BWDS+1) >> 1)
#define BN_CHARS		((MAX_BYTS<<1) + 1)


#include <stdint.h>

inline uint64_t compute_np0(uint64_t n0)
{
	uint64_t inv64 = n0*(n0*n0+14);
	inv64 = inv64*(inv64*n0+2);
	inv64 = inv64*(inv64*n0+2);
	inv64 = inv64*(inv64*n0+2);
	inv64 = inv64*(inv64*n0+2);
	return inv64;
}

#endif//end of __GLOBAL_H__