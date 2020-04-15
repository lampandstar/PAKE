/**
 * srp.h
 * @fileOverview SRP-3 implementation
 * @author weir007
 */
#ifndef __SRP_H__
#define __SRP_H__

#define SRP_OK		0
#define SRP_FAIL	1
#define SRP_ERR		-1

#include "global.h"
#include "params.h"
#include "avx_bn.h"

#define COMMON_MSG_CHARS	65//sha256.output_byts*2 + '\0'

#ifdef __cplusplus
extern "C" {
#endif

API int cpu_srp_init();

/**
 * {A=g**a, v=DB.select(UserID=C)} => {B=v+g**b, u, S=(A·v**u)**b}
 */
API int cpu_srp_server_compute1(OUT char B[BN_CHARS], OUT char u[BN_CHARS], OUT char S[BN_CHARS], IN char A[BN_CHARS], IN char v[BN_CHARS]);

API int cpu_srp_server_compute2(OUT char M2[COMMON_MSG_CHARS], IN char A[BN_CHARS], IN char B[BN_CHARS], IN char K[COMMON_MSG_CHARS], IN char M1[COMMON_MSG_CHARS]);

#ifdef __cplusplus
}
#endif

#endif//end of __SRP_H__