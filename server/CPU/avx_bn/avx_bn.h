/**
 * @fileOverview finite field big integer implementation with AVX512
 * @author weir007
 */
#ifndef __AVX_BN_H__
#define __AVX_BN_H__

#include <stdint.h>
#include <immintrin.h>
#include "global.h"

#define TERM_DIGITS		8
#define TERM_BITS		(52 * TERM_DIGITS)//416
#define DIGITS			((MAX_BITS + 51) / 52)
#define TERMS			((MAX_BITS + (TERM_BITS-1)) / TERM_BITS)


typedef __m512i bn[TERMS];

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化接口，先调这个 */
void bn_init();

/**
 * 数据输入/输出
 */
void data_print(IN uint8_t a[MAX_BYTS]);

void bn_print(IN bn a);

void bin_read_hex(OUT uint8_t r[MAX_BYTS], IN char * str);

void bn_read_hex(OUT bn r, IN char * str);

void bn_to_hex(OUT char r[BN_CHARS], IN bn a);

void bn_load_bin(OUT bn dst, IN uint8_t arr[MAX_BYTS]);

#define bn_load(a, b)\
	bn_load_bin((a), (const uint8_t *)(b))

void bn_set_d(OUT bn r, IN uint64_t a);

void bn_copy(OUT bn dst, IN bn src);


/**
 * 有限域运算
 */
void bn_madd(OUT bn r, IN bn a, IN bn b);

void bn_msub(OUT bn r, IN bn a, IN bn b);

void bn_mmul(OUT bn r, IN bn a, IN bn b);

void bn_mmul_d(OUT bn r, IN bn a, IN uint64_t b);

void bn_mpow(OUT bn r, IN bn a, IN bn b);

void bn_minv(OUT bn r, IN bn a);

#ifdef __cplusplus
}
#endif

#endif//AVX_BN_H__