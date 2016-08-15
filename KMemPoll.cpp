#include "KMemPoll.h"
#include "ikcp.h"

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
}


KThreadMemPoll::~KThreadMemPoll()
{
	TLSSET(s_tlsVal,NULL);
	ikcp_allocator(NULL,NULL);
}






