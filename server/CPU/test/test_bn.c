#include <avx_bn.h>
#include <params.h>
#include <stdio.h>
#include <stdint.h>

void rand_bytes(uint8_t *r, size_t count)
{
	while (count--)
	{
#if 1
		r[count] = rand() & 0xFF;
#else
		r[count] = count & 0xFF;
#endif
	}
}

static uint8_t a[MAX_BYTS], b[MAX_BYTS], n[MAX_BYTS];


void test_load()
{
	char str[MAX_BYTS*2+1] = {0};
	uint8_t data[MAX_BYTS];
	bn sample1, sample2; 
	
	puts("===================== test load =====================");
	
	for (int i=0; i<MAX_BYTS; i++)
	{
		data[i] = i & 0xff;
	}
	puts("resource data:");
	data_print(data);
	
	puts("---------------------------------------------");
	
	puts("sample from bin:");
	bn_load_bin(sample1, data);
	bn_print(sample1);
	
	
	puts("---------------------------------------------");
	
	for (int i=0; i<MAX_BYTS; i++)
		sprintf(str+i*2, "%X%X", (data[MAX_BYTS-i-1] >> 4) & 0xF, data[MAX_BYTS-i-1] & 0xF);
	printf("HEX : %s\n", str);
	
	puts("sample from hex:");
	bn_read_hex(sample2, str);
	bn_print(sample2);
}

void test_output()
{
	bn at;
	char hex[BN_CHARS];
	
	puts("\n===================== test output ======================");
	
	bn_load_bin(at, a);
	puts("bn_print :");
	bn_print(at);
	puts("bn_to_hex :");
	bn_to_hex(hex, at);
	printf("%s\n", hex);
}

void test_madd()
{
	bn at, bt, ct;
	
	puts("\n===================== test add ======================");
	bn_load_bin(at, a);
	bn_load_bin(bt, b);
	puts("sample a :");
	bn_print(at);
	puts("sample b :");
	bn_print(bt);
	
	bn_copy(ct, at);
	
	for (int i=0; i<30; i++)
	{
		printf("----------------------a + b*%d -----------------------\n", i+1);
		bn_madd(ct, ct, bt);
		bn_print(ct);
	}
}

void test_msub()
{
	bn at, bt, ct;
	
	puts("\n===================== test sub ======================");
	bn_load_bin(at, a);
	bn_load_bin(bt, b);
	puts("sample a :");
	bn_print(at);
	puts("sample b :");
	bn_print(bt);
	
	//bn_copy(ct, at);
	bn_copy(ct, bt);
	
	for (int i=0; i<30; i++)
	{
		printf("----------------------a - b*%d -----------------------\n", i+1);
		//bn_msub(ct, ct, bt);
		bn_msub(ct, ct, at);
		bn_print(ct);
	}
}

void test_mmul()
{
	bn at, bt, ct;
	
	puts("\n===================== test mmul =====================");
	bn_load_bin(at, a);
	bn_load_bin(bt, b);
	puts("sample a :");
	bn_print(at);
	puts("sample b :");
	bn_print(bt);

	
	puts("---------------------mul---------------------");
#if 1
	bn_mmul(ct, at, bt);
#else
	bn_copy(ct, at);
	for (int i=0; i<100000000; i++)
		bn_mmul(ct, ct, bt);
#endif
	puts("sample_c :");
	bn_print(ct);
}

void test_mpow()
{
	bn at, bt, ct;
	
	puts("\n===================== test mpow =====================");
	bn_load_bin(at, a);
	bn_load_bin(bt, b);
	puts("sample a :");
	bn_print(at);
	puts("sample b :");
	bn_print(bt);

	puts("---------------------pow---------------------");
#if 1
	bn_mpow(ct, at, bt);
#else
	bn_copy(ct, at);
	for (int i=0; i<1000; i++)	
		bn_mpow(ct, ct, bt);
#endif
	puts("sample_c :");
	bn_print(ct);
}

int test_minv()
{
	bn at, bt, ct;
	
	puts("\n===================== test minv =====================");
	bn_load_bin(at, a);
	bn_load_bin(bt, b);
	puts("sample a :");
	bn_print(at);
	puts("sample b :");
	bn_print(bt);

	puts("---------------------inv---------------------");

	bn_minv(ct, at);
	puts("A**-1 mod N :");
	bn_print(ct);
	
	bn_minv(ct, bt);
	puts("B**-1 mod N :");
	bn_print(ct);
}

#include "global.h"

int main()
{
	rand_bytes(a, MAX_BYTS);
	rand_bytes(b, MAX_BYTS);
	//rand_bytes(n, MAX_BYTS);
	//n[0] |= 1;
	bin_read_hex(n, P);

	puts("a :");
	data_print(a);
	puts("b :");
	data_print(b);
	puts("n :");
	data_print(n);

	bn_init();
	test_load();
	test_output();
	test_madd();
	test_msub();
	test_mmul();
	test_mpow();
	test_minv();
	
	return 0;
}