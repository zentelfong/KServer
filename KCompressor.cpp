#include "KCompressor.h"


KCompressor::KCompressor(void)
{
	m_lz4=LZ4_createStream();
}


KCompressor::~KCompressor(void)
{
	LZ4_freeStream(m_lz4);
}

int KCompressor::Update(const char* source,int inputSize,char* dest,int outMaxSize)
{
	return LZ4_compress_limitedOutput_continue(m_lz4,source,dest,inputSize,outMaxSize);
}