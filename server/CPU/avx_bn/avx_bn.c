/**
 * @fileOverview GF(p) implementation with AVX512
 * @author weir007
 */

#include <string.h>
#include <stdio.h>
#include "avx_bn.h"

#define ALIGN(a, x)		a __attribute__ ((aligned (x)))

/**
 * 内部数据结构&方法
 */
typedef __m512d bf[TERMS];

/**
 * Inner APIs
 */
static inline double make_double(IN uint64_t l);

static inline void make_doubles(OUT double r[DIGITS], IN bn a);

static inline void int2dpf(OUT bf r, IN bn a);

static inline uint64_t compute_np0(uint64_t n0);

static inline uint64_t make_initial(int low, int high);

static void mont_mul(OUT bn r, IN bf a, IN double b[DIGITS], IN const bf n);



/**
 * 公共参数，为防止反复赋值，应先调用初始化接口
 */

static uint64_t np0;
static bn pn, pr, prsqr;
				  
static __m512i _zero, _ones, _sixteen;
static __m512d _c1, _c2;
#if 0
static __m512i _rotate_up, _rotate_down;
#endif

/* 初始化接口，必须先调用 */
void bn_init()
{
	_zero = _mm512_setzero_si512();
	_ones = _mm512_set1_epi64(MASK_ALL_ONE);
	_sixteen = _mm512_set1_epi64(make_initial(16, 16));
	_c1 = _mm512_set1_pd(make_double(0x4670000000000000ull));
	_c2 = _mm512_set1_pd(make_double(0x4670000000000001ull));
#if 0
	_rotate_up = _mm512_setr_epi64(7, 8, 9, 10, 11, 12, 13, 14);
	_rotate_down = _mm512_setr_epi64(1, 2, 3, 4, 5, 6, 7, 8);
#endif
	bn_read_hex(pn, P);
	bn_read_hex(pr, R);
	bn_read_hex(prsqr, R2);
	np0 = compute_np0((uint64_t)_mm_cvtsi128_si64(_mm512_castsi512_si128(pn[0])));
}



/**
 * utils
 */
void bin_read_hex(OUT uint8_t r[MAX_BYTS], IN char * str)
{
	size_t len = strlen(str);
	char ch;
	uint8_t b;
	int i, j;
	
	memset(r, 0, MAX_BYTS);
	
	for (i=len-1; i>=0; i--)
	{
		j = (len - 1 - i) >> 1;
		ch = str[i];
		if (ch>='0' && ch<='9')
			b = ch - '0';
		else if (ch>='a' && ch<='f')
			b = ch - 'a' + 0xA;
		else if (ch>='A' && ch<='F')
			b = ch - 'A' + 0xA;
		else
		{
			puts("Input value error.");
			return;
		}
		
		r[j] >>= 4;
		r[j] |= b << 4;
	}
	if (len & 0x1)
	{
		r[j] >>= 4;
	}
}
 
 
void bn_read_hex(OUT bn r, IN char * str)
{
	size_t len = strlen(str);
	uint8_t bin[MAX_BYTS] = {0};
	
	bin_read_hex(bin, str);
	
	bn_load_bin(r, bin);
}

void bn_to_hex(OUT char r[BN_CHARS], IN bn a)
{
	int i, j = MAX_BYTS%52 >> 3;
	uint64_t * t = (uint64_t *)(a+TERMS-1);
	char * p = r;
		
	sprintf(p, "%llx", t[j--] & MASK_ALL_ONE);
	p += strlen(r);//or (MAX_BITS % 52 + 3)>>1
	
	for (; j>=0; j--)
	{
		sprintf(p, "%013llx", t[j] & MASK_ALL_ONE);
		p += 13;
	}
	for (i=TERMS-2; i>=0; i--)
	{
		t = (uint64_t *)(a + i);
		for (j=TERM_DIGITS-1; j>=0; j--)
		{
			sprintf(p, "%013llx", t[j] & MASK_ALL_ONE);
			p += 13;
		}
	}
	r[BN_CHARS-1] = 0;
}

/**
 * load bn from src
 */
void bn_load_bin(OUT bn dst, IN uint8_t src[MAX_BYTS])
{
	/*
	 * 416 = (512/64) * 52 = 52 byte
	 */
	int i, j = 0;
	__m512i permute = _mm512_setr_epi64(
		0x0003000200010000,0x0006000500040003,
		0x0009000800070006,0x000c000b000a0009,
		0x0010000f000e000d,0x0013001200110010,
		0x0016001500140013,0x0019001800170016),
		      shift = _mm512_setr_epi64(0, 4, 8, 12, 0, 4, 8, 12), data;
			  //_ones = _mm512_set1_epi64(0x000FFFFFFFFFFFFF)

			  
	for (i=0; i<TERMS-1; i++)
	{
		/* 52B => 52b * 8 */
		data = _mm512_maskz_loadu_epi8(0x1FFFFFFFFFFFFF, src + j);
		data = _mm512_permutexvar_epi16(permute, data);
		data = _mm512_srlv_epi64(data, shift);
		dst[i] = _mm512_and_si512(data, _ones);
		j += TERM_BITS >> 3;
	}
	//tail
	data = _mm512_maskz_loadu_epi8((1<<MAX_BYTS%52)-1, src + j);
	data = _mm512_permutexvar_epi16(permute, data);
	data = _mm512_srlv_epi64(data, shift);
	dst[i] = _mm512_and_si512(data, _ones);
}

/**
 * Set bn with integer
 * b = a
 */
void bn_set_d(OUT bn r, IN uint64_t a)
{
	int i;
	r[0] = _mm512_setr_epi64(a, 0, 0, 0, 0, 0, 0, 0);
	for (i=1; i<TERMS; i++)
		r[i] = _mm512_setzero_si512();
}


void data_print(IN uint8_t a[MAX_BYTS])
{
	for (int i=MAX_BYTS-1; i>=0; i--)
	{
		printf("%02X", a[i]);
	}
	printf("\n");
}

void term_print(IN __m512i a)
{
	int i;
	uint64_t * t = (uint64_t *)&a;
	for (i=TERM_DIGITS-1; i>=0; i--)
	{
		printf("%016lX ", t[i]);
	}
	printf("\n");
}

void bn_print(IN bn a)
{
	int i, j = MAX_BYTS%52 >> 3;
	uint64_t * t = (uint64_t *)(a+TERMS-1);
	
	printf("%llX", t[j--] & MASK_ALL_ONE);
	
	for (; j>=0; j--)
	{
		printf("%013llX", t[j] & MASK_ALL_ONE);
	}
	for (i=TERMS-2; i>=0; i--)
	{
		t = (uint64_t *)(a + i);
		for (j=TERM_DIGITS-1; j>=0; j--)
		{
			printf("%013llX", t[j] & MASK_ALL_ONE);
		}
	}
	printf("\n");
}


 
void bn_copy(OUT bn dst, IN bn src)
{
	int i = TERMS;
	while (i-- > 0)
		dst[i] = src[i];
}

int bn_cmp(IN bn a, IN bn b)//非常数时间
{
	int i = TERMS-1, r;
	uint8_t cmp1, cmp2, mask = (1<<DIGITS%TERM_DIGITS) - 1;
	cmp1 = _mm512_mask_cmp_epu64_mask(mask, a[TERMS-1], b[TERMS-1], _MM_CMPINT_LT);
	cmp2 = _mm512_mask_cmp_epu64_mask(mask, b[TERMS-1], a[TERMS-1], _MM_CMPINT_LT);
	
	while (cmp1==cmp2 && i-->0)
	{
		cmp1 = _mm512_cmp_epu64_mask(a[i], b[i], _MM_CMPINT_LT);
		cmp2 = _mm512_cmp_epu64_mask(b[i], a[i], _MM_CMPINT_LT);	
	}

	return cmp1==cmp2 ? CMP_EQ : cmp1 > cmp2 ? CMP_LT : CMP_GT;
}


/**
 * r = a + b
 */
static void bn_add(OUT bn r, IN bn a, IN bn b)
{
	__m512i c0 = _zero,  c1, t;
	int i;
	
	for (i=0; i<TERMS; i++)
	{
		/**
		 * carry is kept in form: [xx, xx, xx, ... , {0,1}]
		 */
		
		/* r[i] = a[i] + b[i] + carry */
		t = _mm512_add_epi64(a[i], b[i]);
		t = _mm512_mask_add_epi64(t, 0x1, t, c0);//absorb carry from last term
		/* inner carry absorb */
		c0 = _mm512_srli_epi64(t, DIGIT_BITS);//c : [{0,1}, {0,1}, ... ,{0,1}]
		c0 = _mm512_alignr_epi64(c0, c0, 7);//c: [c_7, c_6, ..., c_1, c_next]
		t = _mm512_and_epi64(t, _ones);
		t = _mm512_mask_add_epi64(t, 0xfe, t, c0);//allow carry-holding?
		/* thoroughly absorb */
		c1 = _mm512_srli_epi64(t, DIGIT_BITS);
		c1 = _mm512_alignr_epi64(c1, c1, 7);//c: [c_7, c_6, ..., c_1, c_next]
		t = _mm512_and_epi64(t, _ones);
		r[i] = _mm512_mask_add_epi64(t, 0xfe, t, c1);//allow carry-holding?
		c0 = _mm512_mask_add_epi64(c0, 0x1, c1, c0);		
	}
}

/**
 * r = a - b
 * assumes that a >= b
 */
static void bn_sub(OUT bn r, IN bn a, IN bn b)
{
	uint8_t mask = 0xff;
	int i, cmp;
	bn itemp;
	__m512i tail_mask = _mm512_maskz_set1_epi64((1<<DIGITS%TERM_DIGITS)-1, MASK_ALL_ONE);

	for (i=0; i<TERMS; i++)
	{
		itemp[i] = b[i];
		cmp = 0xFF ^ _mm512_cmp_epu64_mask(itemp[i], _zero, _MM_CMPINT_EQ);
		if (cmp == 0x0) continue;
		
		while (cmp & 0x1 == 0)
		{
			mask <<= 1;
			cmp >>= 1;
		}
		//complement
		itemp[i] = _mm512_maskz_xor_epi64(mask, itemp[i], _ones);
		mask = 0x100 - mask;
		itemp[i] = _mm512_mask_add_epi64(itemp[i], mask, itemp[i], _mm512_set1_epi64(1));
		break;
	}

	while (i++ < TERMS-1)
	{
		itemp[i] = _mm512_xor_si512(b[i], _ones);
	}

	bn_add(r, a, itemp);
	r[TERMS-1] = _mm512_and_si512(r[TERMS-1], tail_mask);
}

/**
 * r = a + b mod n
 */
void bn_madd(OUT bn r, IN bn a, IN bn b)
{
	bn_add(r, a, b);	

	while (bn_cmp(r, pn) >= 0)//应使用barrett
	{
		bn_sub(r, r, pn);
	}
}

/**
 * r = a - b mod n
 */
void bn_msub(OUT bn r, IN bn a, IN bn b)
{
	uint8_t mask = 0xff;
	int i, cmp;
	bn itemp;
	
	bn_copy(itemp, a);
	
	while (bn_cmp(itemp, b) < 0)
	{
		bn_add(itemp, itemp, pn);
	}
	
	bn_sub(r, itemp, b);
	
	while (bn_cmp(r, pn) >= 0)
	{
		bn_sub(r, r, pn);
	}
}



/*********************************************\
 * 模乘运算
\*********************************************/

static inline double make_double(IN uint64_t l)
{
	union
	{
		uint64_t l;
		double   d;
	} convert;
	convert.l = l;
	return convert.d;
}

static inline void make_doubles(OUT double r[DIGITS], IN bn a)
{
	int i, j;
	uint64_t * pa;
	
	for (i=0; i<TERMS-1; i++)
	{
		pa  = (uint64_t *)(a + i);
		for (j=0; j<TERM_DIGITS; j++)
		{
			r[i*TERM_DIGITS+j] = (double)pa[j];
		}
	}
	
	pa  = (uint64_t *)(a + TERMS-1);
	for (j=0; j<DIGITS%TERM_DIGITS; j++)
	{
		r[(TERMS-1)*TERM_DIGITS+j] = (double)pa[j];
	}
}

static inline void int2dpf(OUT bf r, IN bn a)
{
	int i;
	for (i=0; i<TERMS; i++)
	{
		r[i] = _mm512_cvtepi64_pd(a[i]);
	}
}


static inline uint64_t compute_np0(uint64_t n0)
{
	uint64_t inv64 = n0*(n0*n0+14);
	inv64 = inv64*(inv64*n0+2);
	inv64 = inv64*(inv64*n0+2);
	inv64 = inv64*(inv64*n0+2);
	inv64 = inv64*(inv64*n0+2);
	return inv64;
}

static inline uint64_t make_initial(int low, int high)
{
	uint64_t x = low*0x433 + high*0x467;
	return 0 - (x<<52);
}

/**
 * r = montgomery_reduce(a * b mod n)
 */
static void mont_mul(OUT bn r, IN bf a, IN double b[DIGITS], IN const bf n)
{
	bn low, high;
	__m512i itemp;
	__m512d bi, qi, dtemp, pl, pbh, pqh;
	
#if 0//公共参数，不宜反复生成
	__m512i _zero = _mm512_setzero_si512(),
	        _ones = _mm512_set1_epi64(MASK_ALL_ONE),
	     _sixteen = _mm512_set1_epi64(make_initial(16, 16)),
	 _rotate_down = _mm512_setr_epi64(1, 2, 3, 4, 5, 6, 7, 8),
	   _rotate_up = _mm512_setr_epi64(7, 8, 9, 10, 11, 12, 13, 14);

	__m512d _c1 = _mm512_set1_pd(make_double(0x4670000000000000ull)),
		    _c2 = _mm512_set1_pd(make_double(0x4670000000000001ull));
#endif
	
	int term, idx;
	uint64_t q;
	
	low[0] = _mm512_setr_epi64(//?
	  make_initial(2, 0),
	  make_initial(4, 2),
	  make_initial(6, 4),
	  make_initial(8, 6),
	  make_initial(10, 8),
	  make_initial(12, 10),
	  make_initial(14, 12),
	  make_initial(16, 14)
	);

	for (term=1; term<TERMS-1; term++)
	{
		low[term] = _mm512_add_epi64(low[term-1], _sixteen);
	}
	low[TERMS-1] = _mm512_maskz_add_epi64((1<<DIGITS%TERM_DIGITS)-1, low[TERMS-2], _sixteen);
	
	for (idx=0; idx<DIGITS; idx++)
	{
		bi = _mm512_set1_pd(b[idx]);
		
		/**
		 ********* (hi, lo) += a*bi *********
		 */
		/* term[0] */
		/* (hi, lo) = a * b[idx] : a.term[0] */
		pbh = _mm512_fmadd_round_pd(a[0], bi, _c1, _MM_FROUND_TO_ZERO|_MM_FROUND_NO_EXC);
		dtemp = _mm512_sub_pd(_c2, pbh);
		pl = _mm512_fmadd_round_pd(a[0], bi, dtemp, _MM_FROUND_TO_ZERO|_MM_FROUND_NO_EXC);
		low[0] = _mm512_add_epi64(low[0], _mm512_castpd_si512(pl));
		//pbh : hi(a.term[0] * b[idx]) + 0x4670000000000000ull
		//low : lo(a.term[0] * b[idx]) - 0x4330000000000000ull
		
		/* montgomery */
		q = (uint64_t)_mm_cvtsi128_si64(_mm512_castsi512_si128(low[0]));
		q = (q * np0) & MASK_ALL_ONE;//qi = n*np0
		qi = _mm512_set1_pd((double)q);
		//(pqh, pl) = S + qi*(a*b[idx])
		pqh = _mm512_fmadd_round_pd(n[0], qi, _c1, _MM_FROUND_TO_ZERO|_MM_FROUND_NO_EXC);
		dtemp = _mm512_sub_pd(_c2, pqh);
		high[0] = _mm512_add_epi64(_mm512_castpd_si512(pbh), _mm512_castpd_si512(pqh));
		pl = _mm512_fmadd_round_pd(n[0], qi, dtemp, _MM_FROUND_TO_ZERO|_MM_FROUND_NO_EXC);
		low[0] = _mm512_add_epi64(low[0], _mm512_castpd_si512(pl));//now low[0].d[0] = 0 mod 2^52

		//(hi, lo) >>= 1 digit
		itemp = _mm512_srli_epi64(low[0], 52);//itemp.d[0] = carry of (low[0].d[0])
		high[0] = _mm512_mask_add_epi64(high[0], 0x1, high[0], itemp);
		/* term[1...TERMS-1] */
		for (term=1; term<TERMS; term++)
		{
			/* accumulate a[term]*bi */
			pbh = _mm512_fmadd_round_pd(a[term], bi, _c1, _MM_FROUND_TO_ZERO|_MM_FROUND_NO_EXC);
			dtemp = _mm512_sub_pd(_c2, pbh);
			pl = _mm512_fmadd_round_pd(a[term], bi, dtemp, _MM_FROUND_TO_ZERO|_MM_FROUND_NO_EXC);
			low[term] = _mm512_add_epi64(low[term], _mm512_castpd_si512(pl));
			/* accumulate qi*n[term] */
			pqh = _mm512_fmadd_round_pd(n[term], qi, _c1, _MM_FROUND_TO_ZERO|_MM_FROUND_NO_EXC);
			dtemp = _mm512_sub_pd(_c2, pqh);
			high[term] = _mm512_add_epi64(_mm512_castpd_si512(pbh), _mm512_castpd_si512(pqh));
			pl = _mm512_fmadd_round_pd(n[term], qi, dtemp, _MM_FROUND_TO_ZERO|_MM_FROUND_NO_EXC);
			low[term] = _mm512_add_epi64(low[term], _mm512_castpd_si512(pl));
			
			//low[term-1] := {low[term].d[0], low[term-1].d[7],...,low[term-1].d[1]}
			//or : low[term-1] = _mm512_alignr_epi64(low[term], low[term-1], 1)
#if 0
			low[term-1] = _mm512_permutex2var_epi64(low[term-1], _rotate_down, low[term]);
#else
			low[term-1] = _mm512_alignr_epi64(low[term], low[term-1], 1);
#endif	
			low[term-1] = _mm512_add_epi64(low[term-1], high[term-1]);
			}
#if 0
			low[TERMS-1] = _mm512_maskz_permutexvar_epi64((1<<(DIGITS%TERM_DIGITS-1))-1, _rotate_down, low[TERMS-1]);
#else
		low[TERMS-1] = _mm512_maskz_alignr_epi64((1<<(DIGITS%TERM_DIGITS-1))-1, itemp, low[TERMS-1], 1);
#endif
		low[TERMS-1] = _mm512_maskz_add_epi64((1<<DIGITS%TERM_DIGITS)-1, low[TERMS-1], high[TERMS-1]);
	}

	itemp = _mm512_setr_epi64(//?
      make_initial(DIGITS*2-2,  DIGITS*2),
      make_initial(DIGITS*2-4,  DIGITS*2-2),
      make_initial(DIGITS*2-6,  DIGITS*2-4),
      make_initial(DIGITS*2-8,  DIGITS*2-6),
      make_initial(DIGITS*2-10, DIGITS*2-8),
      make_initial(DIGITS*2-12, DIGITS*2-10),
      make_initial(DIGITS*2-14, DIGITS*2-12),
      make_initial(DIGITS*2-16, DIGITS*2-14)
    );

	for (term=0; term<TERMS-1; term++)
	{
		low[term] = _mm512_add_epi64(low[term], itemp);
		itemp = _mm512_sub_epi64(itemp, _sixteen);
	}
	low[TERMS-1] = _mm512_maskz_add_epi64((1<<DIGITS%8)-1, low[TERMS-1], itemp);

	/******************** carry process *******************/
	for (term=0; term<TERMS; term++)
	{
		r[term] = _mm512_srli_epi64(low[term], 52);
		low[term] = _mm512_and_epi64(low[term], _ones);
	}

	for (term=TERMS-1; term>0; term--)
	{
#if 0
		r[term] = _mm512_permutex2var_epi64(r[term-1], _rotate_up, r[term]);
#else
		r[term] = _mm512_alignr_epi64(r[term], r[term-1], 7);
#endif
	}
#if 0
	r[0] = _mm512_maskz_permutexvar_epi64(0xFE, _rotate_up, r[0]);
#else
	r[0] = _mm512_alignr_epi64(r[0], _zero, 7);
#endif
	
	for (term=0; term<TERMS; term++)
	{
		low[term] = _mm512_add_epi64(r[term], low[term]);
	}

	for (term=0; term<TERMS; term++)
	{
		r[term] = _mm512_srli_epi64(low[term], 52);
		low[term] = _mm512_and_epi64(low[term], _ones);
    }

    // rotate up
	for (term=TERMS-1; term>0; term--)
	{
#if 0
		r[term] = _mm512_permutex2var_epi64(r[term-1], _rotate_up, r[term]);
#else
		r[term] = _mm512_alignr_epi64(r[term], r[term-1], 7);
#endif
	}
#if 0
	r[0] = _mm512_maskz_permutexvar_epi64(0xFE, _rotate_up, r[0]);
#else
	r[0] = _mm512_alignr_epi64(r[0], _zero, 7);
#endif

    // add in the carry
    for (term=0; term<TERMS; term++)
	{
		r[term] = _mm512_add_epi64(r[term], low[term]);
	}
}

/**
 * r = a * b mod n
 */
void bn_mmul(OUT bn r, IN bn a, IN bn b)
{
	/* compute np0 */
	bf dpf_a, dpf_n;
	bn itemp;
	double dpf_b[DIGITS];
	
	int2dpf(dpf_a, a);
	int2dpf(dpf_n, pn);	

	/* a => mont_form */
	make_doubles(dpf_b, prsqr);
	mont_mul(itemp, dpf_a, dpf_b, dpf_n);
	int2dpf(dpf_a, itemp);
	
	make_doubles(dpf_b, b);
	mont_mul(r, dpf_a, dpf_b, dpf_n);
}

/**
 * r = a * b mod n
 */
void bn_mmul_d(OUT bn r, IN bn a, IN uint64_t b)
{
	/* compute np0 */
	bf dpf_a, dpf_n;
	bn itemp;
	double dpf_b[DIGITS] = {0};
	
	int2dpf(dpf_a, a);
	int2dpf(dpf_n, pn);	

	/* a => mont_form */
	make_doubles(dpf_b, prsqr);
	mont_mul(itemp, dpf_a, dpf_b, dpf_n);
	int2dpf(dpf_a, itemp);
	
	dpf_b[0] = make_double(b);
	mont_mul(r, dpf_a, dpf_b, dpf_n);
}

/**
 * r = R - n
 * R is montgomery parameter, R >= 2**n.bitlen
 */
static inline void compute_rn(OUT bn r, IN bn n)
{
	int i;
	/* 模数n是奇数，因此不必寻找最低1比特，求补码 */
	for (i=0; i<TERMS-1; i++)
	{
		r[i] = _mm512_xor_epi64(r[i], _ones);//r[i] = NOT (r[i])
	}
	r[TERMS-1] = _mm512_maskz_xor_epi64((1<<DIGITS%TERM_DIGITS)-1, r[TERMS-1], _ones);
	r[0] = _mm512_mask_or_epi64(r[0], 0x1, r[0], _mm512_set1_epi64(1));//complement
}

#define WSIZE		4
#define WBORDER		(1<<WSIZE)
/**
 * r = a**b mod n
 */
void bn_mpow(OUT bn r, IN bn a, IN bn b)
{
	
	bn itemp;
	bf dpf_r, dpf_n, dtemp;
	double mont_tb[WBORDER][DIGITS], dbl[DIGITS];
	ALIGN(uint64_t pb[DIGITS], 64);
	size_t idx;
	int i;
	
	for (i=0; i<TERMS-1; i++)
	{
		_mm512_store_epi64(pb+i*TERM_DIGITS, b[i]);
	}
	_mm512_mask_store_epi64(pb+i*TERM_DIGITS, (1<<DIGITS%TERM_DIGITS)-1, b[i]);
	
	/**
	 * precompute table of a**0, a**1, a**((1<<wsize)-1)
	 * default window size = 4
	 */
	int2dpf(dpf_n, pn);
	
	int2dpf(dtemp, a);
	make_doubles(dbl, prsqr);
	mont_mul(itemp, dtemp, dbl, dpf_n);
	int2dpf(dtemp, itemp);
	make_doubles(mont_tb[0], pr);
	for (i=1; i<WBORDER; i++)
	{
		mont_mul(itemp, dtemp, mont_tb[i-1], dpf_n);
		make_doubles(mont_tb[i], itemp);
	}

	//r = MF(1)
	bn_copy(itemp, pr);
	
	for (i=MAX_BITS-WSIZE; i>=0; i-=WSIZE)
	{
		//r = r**16
		int2dpf(dpf_r, itemp);
		make_doubles(dbl, itemp);
		mont_mul(itemp, dpf_r, dbl, dpf_n);
		int2dpf(dpf_r, itemp);
		make_doubles(dbl, itemp);
		mont_mul(itemp, dpf_r, dbl, dpf_n);
		int2dpf(dpf_r, itemp);
		make_doubles(dbl, itemp);
		mont_mul(itemp, dpf_r, dbl, dpf_n);
		int2dpf(dpf_r, itemp);
		make_doubles(dbl, itemp);
		mont_mul(itemp, dpf_r, dbl, dpf_n);
		int2dpf(dpf_r, itemp);
		
		//idx = b[i+WSIZE-1 : i]
		idx = (pb[i/52] >> i%52) & (WBORDER-1);
		
		//r *= mont_tb[idx]
		mont_mul(itemp, dpf_r, mont_tb[idx], dpf_n);
	}
	
	//Recover to normal form
	memset(dbl, 0, sizeof(dbl));
	dbl[0] = 1;
	int2dpf(dpf_r, itemp);
	mont_mul(r, dpf_r, dbl, dpf_n);
}

/**
 * r = a**-1 mod n
 * n must be a prime so that φ(n) = n-1
 * thus a**-1 = a**(n-2)
 */
void bn_minv(OUT bn r, IN bn a)
{
	bn b;
	bn_set_d(b, 2);
	bn_sub(b, pn, b);

	bn_mpow(r, a, b);
}