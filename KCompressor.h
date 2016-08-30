#pragma once
#include "lz4/lz4.h"

class KCompressor
{
public:
	KCompressor(void);
	~KCompressor(void);
	int Update(const char* source,int inputSize,char* dest,int outMaxSize);
private:
	LZ4_stream_t* m_lz4;
};

