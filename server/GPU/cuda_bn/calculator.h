/**
 * @fileOverview finite field big integer implementation with CUDA
 * @author weir007
 */
#ifndef __CUDA_BN_H__
#define __CUDA_BN_H__

#include "global.h"
#include "param.h"

#include "cuda_runtime.h"

/**
 * macros
 */
 
#define devfunc				__device__ __forceinline__ 
 
#define BLOCKS_PER_GRID		1
#define THREADS_PER_BLOCK	128


#define BITS_PER_NUM			MAX_BITS

#define SAMPLES_PER_NUM			((BITS_PER_NUM+51) / 52)
#define THREADS_PER_NUM			4
#define SAMPLES_PER_THREAD		((SAMPLES_PER_NUM+THREADS_PER_NUM-1) / THREADS_PER_NUM)



/**
 * __device__ utils
 */
template<int size, int threads>
devfunc void copy(uint64_t * to, uint64_t * from)
{
	int idx = (threadIdx.x & threads-1) * size;
	
	for (int i=idx; i<idx+size; i++)
	{
		to[i] = from[i];
	}
}

devfunc void make_double(uint64_t l)
{
	union
	{
		uint64_t l;
		double   d;
	} convert;
	convert.l = l;
	return convert.d;
}

/**
 * ((bits + 51) / 52 + samples - 1) / threads = samples
 */
template<int bits, int threads, int samples, int geometry>
class calculator
{
  public:
	/* modulo sampled in 52-bit terms */
	static uint64_t n[samples*threads], mont_r[samples*threads], mont_r2[samples*threads], np0;

	/**
	 * @constructor
	 */
	devfunc calculator()
	{
		memset(op_qw, 0, sizeof(op_qw));
		memset(op_lf, 0, sizeof(op_lf));
	}
	
	devfunc calculator(uint64_t v)
	{
		calculator();
		if (threadIdx.x % threads == 0)
		{
			op_qw[0] = op_lf[0] = v & MASK52;
			op_qw[1] = op_lf[1] = v >> 52;
		}
	}
	
	devfunc calculator(const char * hex)
	{
		
	}
	
	devfunc void set_operand(const char * hex)
	{
		
	}
	
	devfunc void set_operand(uint64_t * bin)
	{
		
	}
	
	devfunc void rand_operand()
	{
		
	}

	devfunc ~calculator()
	{
		/**
		 * clear parameters
		 */
	}
	
	
	/**
	 * r = a + b mod n
	 */
	devfunc void madd(char *hex)
	{
		
	}
	
	/**
	 * r = a - b mod n
	 */
	devfunc void msub(uint64_t * b)
	{
		
	}
	
	/**
	 * r = a * b mod n
	 */
	devfunc void mmul(uint64_t * b)
	{
		
	};
	
	/**
	 * r = a**-1 mod n
	 */
	devfunc void minv()
	{
		
	}

  private:
	uint64_t op_qw[samples];
	double op_lf[samples];
	
	/*  */
	
	devfunc void unify(int dir)
	{
		
	}
	
	devfunc void load_samples(uint64_t * sample, uint64_t * bin)
	{
		__shared__ uint64_t shared_buffer[(bits+63>>6)*geometry/threads];
		int term = threadIdx.x & threads-1;
		
		uint64_t * instance = shared_buffer + threadIdx.x / threads * (bits+63 >> 6);
		/**
		 * copy bin to shared_buffer
		 */
		copy(instance, bin);
		/** 
		 * need thread-sync here? 
		 * now shared_buffer[instance_pos...instance_pos+bits/64] holds current instance
		 */
		int bit_offset = term*samples*52, qw0, qw1, bit_shr;
		
		
		for (int i=0; i<samples; i++)
		{
			/**
			 * instance[0...bits/64] --> sample[0...samples*threads-1]
			 * bitOffset = (term*samples + i) * 52
			 * qwOffset = {bitOffset/52, (bitOffset+51)/52}
			 * 
			 */
			qw0 = bit_offset >> 6;
			qw1 = (bit_offset+51) >> 6;
			/**
			 * b = bit_offset % 52;
			 * sample[term*samples+i] = qw0 == qw1?
			 * instance[qw0]>>b : 
			 * (instance[qw0]>>b) | (instance[qw1]<<64-b);
			 * sample[term*samples+i] &= 0xfffffffffffffull;
			 */
			bit_shr = bit_offset & 0x3f;
			sample[i] = instance[qw0] >> bit_shr;
			sample[i] |= instance[qw1] << (64-bit_shr);
			sample[i] &= MASK52;
			bit_offset += 52;
		}
	}

	/**
	 * load instance from hex,
	 * note : hex shall be zero-filled, that is, 
	 * strlen(hex) = BN_CHARS-1 = (bits+3)/4,
	 * ending with '\0'
	 */
	devfunc void load_samples(uint64_t * sample, const char * hex)
	{
		/**
		 * unneccesary to use shared memory
		 * hex may either be zero-filled or not
		 * each thread holds SAMPLES_PER_THREAD*13 hex char
		 */
		assert(strlen(hex) == BN_CHARS-1);
		/**
		 * hex is Big-Endian, while sample needs Small-Endian
		 */
		size_t hex_per_thread = samples * 13;
		int term = threadIdx.x & threads-1,
		    //bit_idx = term * samples,
			hex_idx = (threads - term) * hex_per_thread - 1;
		
		const char * from = hex + hex_idx;
		//uint64_t * to = sample + bit_idx;

		/* in case hex is not zero-filled */
		for (int pos=0,i=hex_per_thread-1; i>=0; i--, pos+=4)
		{
			char ch = from[i];
			uint64_t b = 0;
			
			if (ch>='0' && ch<='9')
				b = ch - '0';
			else if (ch>='a' && ch<='f')
				b = ch - 'a' + 0xA;
			else if (ch>='A' && ch<='F')
				b = ch - 'A' + 0xA;
			
			sample[pos/52] |= b << (pos % 52);
		}
	}
	
	devfunc void load_samples(double * sample, uint64_t * bin)
	{
		load_samples((uint64_t *)sample, bin);
	}
	
	devfunc void load_samples(double * sample, const char * hex)
	{
		load_samples((uint64_t *)sample, hex);
	}
	
	/**
	 * store r
	 */
	devfunc void store_samples()
	{
		
	}

}

/**
 * __host__ 函数
 */
inline void load_hex(OUT uint64_t r[SAMPLES_PER_NUM], IN char * str)
{
	size_t len = strlen(str);
	char ch;
	uint64_t b;
	int i, j, pos = 0;

	memset(r, 0, SAMPLES_PER_NUM*sizeof(uint64_t));
	
	for (i=len-1; i>=0; i--, pos+=4)
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
		
		/* insert */
		r[pos/52] |= b << (pos % 52);
	}
}

/**
 * 对模数及montgomery参数的初始化应在CPU中完成
 */
uint64_t calculator::n[], calculator::mont_r[], calculator::mont_r2[], calculator::np0;

__host__ void cuda_init()
{
	/**
	 * initialize modulo and montgomery parameters
	 */
	load_hex(calculator::n, P);
	load_hex(calculator::mont_r, R);
	load_hex(calculator::mont_r2, R2);
	calculator::np0 = compute_np0(calculator::n[0]);
	
	/* select device */
	cudaSetDevice(0);
}

#if 0
/**
 * invoke demo
 */
 
API int gpu_srp_init()
{
	cuda_init();
	
	return SRP_OK;
}

/**
 * given A, v
 *  b = rand(), u = rand()
 *  B = v + g**b
 *  S = (A*v**u)**b
 * return b, u, B, S 
 * suppose hash-related values, such as M2 and K,
 * are computed in http-server
 */
__global__ void gpu_srp_compute_func(char * obuf, char * ibuf)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	char * in = ibuf + idx*BN_CHARS*2;
	
	load_hex(A, in);//在thread中调用load_hex，减轻压力
	load_hex(v, in+BN_CHARS);//在thread中调用load_hex，减轻压力
	g = G;
	
	calculator<BITS_PER_NUM, THREADS_PER_NUM, SAMPLES_PER_THREAD, THREADS_PER_BLOCK> calc;

	/**
	 * 1. b = rand()
	 * 2. B = v+ g**b
	 */
	calc.load_op0(G);
	b = rand();
	calc.load_op1(b);
	calc.mpow();
	calc.load_op1(v);//到底是传v，还是传v+(threadIdx%THREADS_PER_NUM)*SAMPLES_PER_THREAD
	calc.madd();
	calc.store(obuf + idx*BN_CHARS*3);
	
	/**
	 * 3. u = rand()
	 * 4. S = (A·v**u)**b
	 */
	calc.clear_ops();
	u = rand();
	copy u to obuf + idx*BN_CHARS*3 + BN_CHARS;
	calc.load_op0(v);
	calc.load_op1(u);
	calc.mpow();
	calc.load_op1(A);
	calc.mmul();
	calc.load_op1(b);
	calc.mpow();
	calc.store(obuf + idx*BN_CHARS*3 + 2*BN_CHARS);
}

/**
 * 并行运行count个SRP
 * ibuf 由 count 个 A|v 组成，但需要固定长度：
 * char A[BN_CHARS] | char v[BN_CHARS], socket的buf预留了3*BN_CHARS大小
 */
API __host__ int gpu_srp_get_resp((OUT char * obuf, IN char *ibuf, IN int count)
{
	cudaMemcpy(gpu_ibuf, ibuf, count*BN_CHARS*2, cudaMemcpyHostToDevice);
	srp_compute_func<<<BLOCKS_PER_GRID, THREADS_PER_BLOCK>>>(obuf, gpu_ibuf);
	cudaMemcpy(obuf, gpu_obuf, count*BN_CHARS*3, cudaMemcpyHostToDevice);
	return 0;
}

#endif

#endif//__CUDA_BN_H__