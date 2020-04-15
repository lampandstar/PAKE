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

#endif//end of __GLOBAL_H__