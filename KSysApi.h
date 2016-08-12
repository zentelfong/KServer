#pragma once

#ifndef WIN32

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>

#else

#ifdef WIN32
#include <winsock2.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif
#include <ws2tcpip.h>
#include <wspiapi.h>


#endif


class KNetInitialized
{
public:
	KNetInitialized()
	{
#ifdef WIN32
		m_wVersionRequested = MAKEWORD(2, 2);
		WSAStartup(m_wVersionRequested, &m_wsaData);
#endif
	}

	~KNetInitialized()
	{
#ifdef WIN32
		WSACleanup();
#endif
	}

private:
#ifdef WIN32
	WORD m_wVersionRequested;
	WSADATA m_wsaData;
#endif
};





