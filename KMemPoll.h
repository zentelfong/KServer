#pragma once
#include "KSysApi.h"
#include "dlmalloc.h"

#ifdef WIN32
#include <assert.h>

#define TLSVAR			DWORD
#define TLSALLOC(k)	    (*(k)=TlsAlloc(), TLS_OUT_OF_INDEXES==*(k))
#define TLSFREE(k)		(!TlsFree(k))
#define TLSGET(k)		TlsGetValue(k)
#define TLSSET(k, a)	(!TlsSetValue(k, a))
#ifdef _DEBUG
static LPVOID ChkedTlsGetValue(DWORD idx)
{
	LPVOID ret=TlsGetValue(idx);
	assert(S_OK==GetLastError());
	return ret;
}
#undef TLSGET
#define TLSGET(k) ChkedTlsGetValue(k)
#endif
#else

#define TLSVAR			pthread_key_t
#define TLSALLOC(k)	    pthread_key_create(k, 0)
#define TLSFREE(k)		pthread_key_delete(k)
#define TLSGET(k)		pthread_getspecific(k)
#define TLSSET(k, a)	pthread_setspecific(k, a)
#endif


//内存池
class KMemPoll
{
public:
	KMemPoll(size_t capacity)
	{
		m_memSpace=create_mspace(capacity,0);
	}

	~KMemPoll()
	{
		destroy_mspace(m_memSpace);
	}

	void* Malloc(size_t bytes)
	{
		return mspace_malloc(m_memSpace,bytes);
	}

	void Free(void* mem)
	{
		mspace_free(m_memSpace,mem);
	}

	void* Realloc(void* mem,size_t bytes)
	{
		return mspace_realloc(m_memSpace,mem,bytes);
	}

	void* Calloc(size_t n_elements, size_t elem_size)
	{
		return mspace_calloc(m_memSpace,n_elements,elem_size);
	}

private:
	mspace m_memSpace;
};

//当前线程的内存池
class KThreadMemPoll:public KMemPoll
{
public:
	KThreadMemPoll(size_t capacity);
	~KThreadMemPoll();

	static KMemPoll* Current()
	{
		return (KMemPoll*)TLSGET(s_tlsVal);
	}

private:
	static TLSVAR s_tlsVal;
};

//重载C++ new和delete函数
class KMalloc
{
public:
	void * operator new(size_t bytes)
	{
		KMemPoll* poll=KThreadMemPoll::Current();
		if (poll)
		{
			return poll->Malloc(bytes);
		}
		else
			return malloc(bytes);
	}

	void * operator new[](size_t bytes)
	{
		KMemPoll* poll=KThreadMemPoll::Current();
		if (poll)
		{
			return poll->Malloc(bytes);
		}
		else
			return malloc(bytes);
	}

	void operator delete(void *mem)
	{
		KMemPoll* poll=KThreadMemPoll::Current();
		if (poll)
		{
			poll->Free(mem);
		}
		else
			free(mem);
	}

	void operator delete[](void *mem)
	{
		KMemPoll* poll=KThreadMemPoll::Current();
		if (poll)
		{
			poll->Free(mem);
		}
		else
			free(mem);
	}
};


//C语言风格内存申请，释放函数
inline void* kMalloc(size_t bytes)
{
	KMemPoll* poll=KThreadMemPoll::Current();
	if (poll)
	{
		return poll->Malloc(bytes);
	}
	else
		return malloc(bytes);
}

inline void kFree(void* mem)
{
	KMemPoll* poll=KThreadMemPoll::Current();
	if (poll)
	{
		poll->Free(mem);
	}
	else
		free(mem);
}

inline void* kRealloc(void* mem,size_t bytes)
{
	KMemPoll* poll=KThreadMemPoll::Current();
	if (poll)
		return poll->Realloc(mem,bytes);
	else
		return realloc(mem,bytes);
}


















