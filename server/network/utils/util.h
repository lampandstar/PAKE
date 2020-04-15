#ifndef __UTIL_H__
#define __UTIL_H__

#define GETU32(x)\
	u[0] | ((uint32_t)x[1]<<8) | ((uint32_t)x[2]<<16) | ((uint32_t)x[3]<<24)

/* API for HTTP server */

		
		
/* other functions */
inline void secure_free(void * p)
{
	free(p);
}


#endif