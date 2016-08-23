#pragma once
#include "KSysApi.h"
#include "kcp/ikcp.h"
#include "KHeap.h"
#include "KHashMap.h"
#include "KMemPoll.h"
#include "KAddr.h"

inline int kSetNonblocking(SOCKET fd)
{
#ifdef WIN32
	{
		unsigned long nonblocking = 1;
		ioctlsocket(fd, FIONBIO, (unsigned long*) &nonblocking);
	}
#else
	{
		int flags;
		if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
			event_warn("fcntl(%d, F_GETFL)", fd);
			return -1;
		}
		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
			event_warn("fcntl(%d, F_SETFL)", fd);
			return -1;
		}
	}
#endif
	return 0;
}

typedef IUINT32 ktime_t;

#ifdef POSIX
inline ktime_t kTime()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#define closesocket close

#endif


#ifdef WIN32
inline ktime_t kTime() {
	return GetTickCount();
}
#endif