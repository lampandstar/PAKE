/**
 * srp.c
 * @fileOverview SRP server side
 * @auther weir007
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "crypto.h"
#include "srp.h"

#define HASH sha256

static bn g;

API int cpu_srp_init()
{
	bn_init();
	bn_set_d(g, G);
	return SRP_OK;
}

/**
 * utils
 */
static int verify(IN char * str1, IN char * str2, size_t len)
{
	uint64_t r = 0;
	size_t i;
	
	for (i=0; i<len; i++)
		r |= str1[i] ^ str2[i];
	r = (0 - r) >> 63;
	return r;
}

static void ub2ch(OUT char * str, IN uint8_t * bin, size_t byts)
{
	size_t i;
	
	memset(str, 0, byts*2+1);
	if (byts == 0)
		return;

	for (i=0; i<byts; i++)
	{
#if 1
		sprintf(str+i*2, "%02x", bin[i]);
#else
		
#endif
	}
}

#include <time.h>

static unsigned int rnd_ctr = 0;

static void rand_bin(uint8_t *r, size_t count)
{
	srand(time(0) ^ ((unsigned long)r & 0xFFFFFFFF) ^ count ^ rnd_ctr++);
	while (count--)
	{
#if 1
		r[count] = rand() & 0xFF;
#else
		r[count] = count & 0xFF;
#endif
	}
}

/**
 * {A=g**a, v=DB.select(UserID=C)} => {B=v+g**b, u, S=(A·v**u)**b}
 */
API int cpu_srp_server_compute1(OUT char B[BN_CHARS], OUT char u[BN_CHARS], OUT char S[BN_CHARS], IN char A[BN_CHARS], IN char v[BN_CHARS])
{
	bn bn_u, bn_b, bn_v, bn_a, bn_tmp;
	uint8_t rnd[MAX_BYTS*2];
	/**
	 * 1. b = rand()
	 * 2. B = v+ g**b
	 */
	rand_bin(rnd, sizeof(rnd));
	bn_load_bin(bn_b, rnd);//小端的
	bn_mpow(bn_b, g, bn_b);
	bn_read_hex(bn_v, v);//大端的
	bn_madd(bn_b, bn_b, bn_v);
	bn_to_hex(B, bn_b);
	/**
	 * 3. u = rand()
	 * 4. S = (A·v**u)**b
	 */
	ub2ch(u, rnd+MAX_BYTS, MAX_BYTS);
	bn_read_hex(bn_u, u);
	bn_mpow(bn_tmp, bn_v, bn_u);
	bn_read_hex(bn_a, A);
	bn_mmul(bn_tmp, bn_tmp, bn_a);
	bn_mpow(bn_tmp, bn_tmp, bn_b);
	bn_to_hex(S, bn_tmp);
	
	return SRP_OK;
}

API int cpu_srp_server_compute2(OUT char M2[COMMON_MSG_CHARS], IN char A[BN_CHARS], IN char B[BN_CHARS], IN char K[COMMON_MSG_CHARS], IN char M1[COMMON_MSG_CHARS])
{
	uint8_t msg[COMMON_MSG_CHARS + BN_CHARS*2 - 2], dgst[32];
	
	/* 1. M2 = H(A, B, K) */
	strcat(msg, A);
	strcat(msg, B);
	strcat(msg, K);
	HASH(dgst, msg, strlen(msg));
	ub2ch(M2, dgst, 32);
	/* 2. M1==M2 ? SUCCEED : FAIL */
	return verify(M1, M2, COMMON_MSG_CHARS);
}