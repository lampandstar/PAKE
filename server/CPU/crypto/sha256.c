/* sha256.c - SHA reference implementation using C            */
/*   Written and placed in public domain by Jeffrey Walton    */

/* xlc -DTEST_MAIN sha256.c -o sha256.exe           */
/* gcc -DTEST_MAIN -std=c99 sha256.c -o sha256.exe  */
#include "crypto.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static const uint32_t K256[] =
{
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};

#define ROTATE(x,y)  (((x)>>(y)) | ((x)<<(32-(y))))
#define Sigma0(x)    (ROTATE((x), 2) ^ ROTATE((x),13) ^ ROTATE((x),22))
#define Sigma1(x)    (ROTATE((x), 6) ^ ROTATE((x),11) ^ ROTATE((x),25))
#define sigma0(x)    (ROTATE((x), 7) ^ ROTATE((x),18) ^ ((x)>> 3))
#define sigma1(x)    (ROTATE((x),17) ^ ROTATE((x),19) ^ ((x)>>10))

#define Ch(x,y,z)    (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x,y,z)   (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

/* Avoid undefined behavior                    */
/* https://stackoverflow.com/q/29538935/608639 */
static inline uint32_t B2U32(uint8_t val, uint8_t sh)
{
    return ((uint32_t)val) << sh;
}

static inline void UBW2B(uint8_t u8[4], uint32_t bw)
{
	u8[3] = bw & 0xFF;
	u8[2] = (bw >> 8) & 0xFF;
	u8[1] = (bw >> 16) & 0xFF;
	u8[0] = (bw >> 24) & 0xFF;
}

/* Process multiple blocks. The caller is responsible for setting the initial */
/*  state, and the caller is responsible for padding the final block.        */
static void sha256_process(uint32_t state[8], const uint8_t data[], uint32_t length)
{
    uint32_t a, b, c, d, e, f, g, h, s0, s1, T1, T2;
    uint32_t X[16], i;

    size_t blocks = length / 64;
    while (blocks--)
    {
        a = state[0];
        b = state[1];
        c = state[2];
        d = state[3];
        e = state[4];
        f = state[5];
        g = state[6];
        h = state[7];

        for (i = 0; i < 16; i++)
        {
            X[i] = B2U32(data[0], 24) | B2U32(data[1], 16) | B2U32(data[2], 8) | B2U32(data[3], 0);
            data += 4;

            T1 = h;
            T1 += Sigma1(e);
            T1 += Ch(e, f, g);
            T1 += K256[i];
            T1 += X[i];

            T2 = Sigma0(a);
            T2 += Maj(a, b, c);

            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }

        for (; i < 64; i++)
        {
            s0 = X[(i + 1) & 0x0f];
            s0 = sigma0(s0);
            s1 = X[(i + 14) & 0x0f];
            s1 = sigma1(s1);

            T1 = X[i & 0xf] += s0 + s1 + X[(i + 9) & 0xf];
            T1 += h + Sigma1(e) + Ch(e, f, g) + K256[i];
            T2 = Sigma0(a) + Maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }
}

void sha256(OUT uint8_t dgst[32], IN uint8_t * msg, IN uint64_t len)
{
	uint64_t bitlen = len << 3, tail = len & 0x3F;
	uint8_t block[64] = {0};
	
	uint32_t state[8] = {
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
	};

	sha256_process(state, msg, len-tail);
	
	//process tail
	memcpy(block, msg+len-tail, tail);
	block[tail++] = 0x80;
			  
	memset(block+tail, 0, 64 - tail);
	
	if (64 - tail < 8)
	{/* one more block required */
		sha256_process(state, block, 64);
		memset(block, 0, 56);
	}
	
	block[56] = (bitlen >> 56) & 0xFF;
	block[57] = (bitlen >> 48) & 0xFF;
	block[58] = (bitlen >> 40) & 0xFF;
	block[59] = (bitlen >> 32) & 0xFF;
	block[60] = (bitlen >> 24) & 0xFF;
	block[61] = (bitlen >> 16) & 0xFF;
	block[62] = (bitlen >> 8)  & 0xFF;
	block[63] =  bitlen        & 0xFF;
	sha256_process(state, block, 64);
	
    /* Save state */
    UBW2B(dgst, state[0]);
    UBW2B(dgst + 4, state[1]);
    UBW2B(dgst + 8, state[2]);
    UBW2B(dgst + 12, state[3]);
    UBW2B(dgst + 16, state[4]);
    UBW2B(dgst + 20, state[5]);
    UBW2B(dgst + 24, state[6]);
    UBW2B(dgst + 28, state[7]);
}