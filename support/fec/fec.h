/**
 * zfec -- fast forward error correction library with Python interface
 *
 * See README.rst for documentation.
 */

#ifndef _FEC_H_
#define _FEC_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


#if defined(_MSC_VER)
	#define alloca _alloca
#else
	#ifdef __GNUC__
		#ifndef alloca
			#define alloca(x) __builtin_alloca(x)
		#endif
	#else
		#include <alloca.h>
	#endif
#endif


typedef unsigned char gf;

typedef struct {
  unsigned long magic;
  unsigned short k, n;                     /* parameters of the code */
  gf* enc_matrix;
} fec_t;

//≥ı ºªØ
void init_fec(void);

void fec_set_allocator(void* (*new_malloc)(size_t), void (*new_free)(void*));

/**
 * param k the number of blocks required to reconstruct
 * param m the total number of blocks created
 */
fec_t* fec_new(unsigned short k, unsigned short m);
void fec_delete(fec_t* p);


void fec_encode(const fec_t* fec, const gf** data_blocks,size_t nrDataBlocks, gf** fec_blocks,size_t nrFecBlocks, size_t blockSize);

typedef struct fec_enc_data 
{
	gf* data;
	size_t len;
}fec_enc_data;

void fec_encode2(const fec_t* fec, const fec_enc_data* data_blocks,size_t nrDataBlocks,gf** fec_blocks,size_t nrFecBlocks,size_t fecBlockSize);

/**
 * @param inpkts an array of packets (size k); If a primary block, i, is present then it must be at index i. Secondary blocks can appear anywhere.
 * @param outpkts an array of buffers into which the reconstructed output packets will be written (only packets which are not present in the inpkts input will be reconstructed and written to outpkts)
 * @param index an array of the blocknums of the packets in inpkts
 * @param sz size of a packet in bytes
 */
void fec_decode(const fec_t* fec, const gf** inpkts, gf** outpkts, const unsigned* index, size_t sz);


typedef struct fec_dec_data 
{
	const gf* data;
	size_t len;
	size_t idx;
}fec_dec_data;

void fec_decode2(const fec_t* fec,const fec_dec_data* data_blocks,gf** out_blocks,size_t blockSize);

#ifdef __cplusplus
}
#endif


#endif