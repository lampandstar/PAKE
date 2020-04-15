/**
 * @fileOverview Some necessary crypto APIs
 * @author weir007
 */
#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#include <stdint.h>

#ifndef IN
#define IN const
#define OUT
#endif

#ifdef __cplusplus
extern "C" {
#endif

void sha256(OUT uint8_t dgst[32], IN uint8_t * data, IN uint64_t len);

#ifdef __cplusplus
}
#endif

#endif