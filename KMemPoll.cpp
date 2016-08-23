#include "KMemPoll.h"
#include "kcp/ikcp.h"
#include "fec/fec.h"

#ifdef WIN32
#pragma comment(lib,"Ws2_32.lib")
#endif


TLSVAR KThreadMemPoll::s_tlsVal=0;

KThreadMemPoll::KThreadMemPoll(size_t capacity)
	:KMemPoll(capacity)
{
	if (s_tlsVal==0)
	{
		TLSALLOC(&s_tlsVal);
	}
	TLSSET(s_tlsVal,this);
	ikcp_allocator(kMalloc,kFree);
	fec_set_allocator(kMalloc,kFree);
}


KThreadMemPoll::~KThreadMemPoll()
{
	TLSSET(s_tlsVal,NULL);
	ikcp_allocator(malloc,free);
	fec_set_allocator(malloc,free);
}






