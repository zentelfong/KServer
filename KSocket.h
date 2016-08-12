#pragma once
#include "KUtil.h"

//UDP socket的封装
class KSocket
{
public:
	KSocket()
	{
		m_socket=::socket(AF_INET, SOCK_DGRAM,0);
		kSetNonblocking(m_socket);
		bool reuseAddr=true;
		::setsockopt(m_socket,SOL_SOCKET,SO_REUSEADDR,(char*)&reuseAddr,sizeof(reuseAddr));
	}

	~KSocket()
	{
		closesocket(m_socket);
	}

	inline int Bind(const sockaddr* addr)
	{
		return ::bind(m_socket,addr,sizeof(sockaddr));
	}

	inline int Sendto(const void *buf, int len, const struct sockaddr *to)
	{
		return ::sendto(m_socket,(const char*)buf,len,0,to,sizeof(sockaddr));
	}

	inline int Recvfrom(void *buf, int len, struct sockaddr *from)
	{
		int addrlen=sizeof(sockaddr);
		return ::recvfrom(m_socket,(char*)buf,len,0,from,&addrlen);
	}

	bool CheckReadable(ktime_t time)
	{
		//select超时检查socket
		fd_set fdRead;
		struct timeval timeout;
		timeout.tv_usec=time*1000;
		timeout.tv_sec=0;

		FD_ZERO(&fdRead); 
		FD_SET(m_socket, &fdRead);
		int nRet = ::select(m_socket+1, &fdRead, NULL, NULL, &timeout );
		if (nRet>0 && FD_ISSET(m_socket, &fdRead))
		{
			return true;
		}
		else
			return false;
	}

private:
	SOCKET   m_socket;
};


