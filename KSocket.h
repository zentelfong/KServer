#pragma once
#include "KUtil.h"

#ifdef WIN32
struct iovec
{
	int iov_len;
	char* iov_base;
};
#endif

//UDP socket的封装
class KSocket
{
public:
	KSocket(int af)
	{
		m_socket=::socket(af, SOCK_DGRAM,0);
		kSetNonblocking(m_socket);
		bool reuseAddr=true;
		::setsockopt(m_socket,SOL_SOCKET,SO_REUSEADDR,(char*)&reuseAddr,sizeof(reuseAddr));
	}

	~KSocket()
	{
		closesocket(m_socket);
	}

	inline int Bind(const KAddr* addr)
	{
		return ::bind(m_socket,addr->sockAddr(),addr->sockAddrLen());
	}

	inline int SendMsg(const iovec* msg,int count,const KAddr* addr)
	{
		int res;
#ifndef WIN32
		msghdr mh;
		mh.msg_name = addr->sockAddr();
		mh.msg_namelen = addr->sockAddrLen();
		mh.msg_iov = msg;
		mh.msg_iovlen = count;
		mh.msg_control = NULL;
		mh.msg_controllen = 0;
		mh.msg_flags = 0;
		res = ::sendmsg(m_iSocket, &mh, 0);
#else
		DWORD size = 0;
		res = ::WSASendTo(m_socket, (LPWSABUF)msg, 2, &size, 0, addr->sockAddr(),addr->sockAddrLen(), NULL, NULL);
		res = (0 == res) ? size : -1;
#endif
		return res;
	}


	inline int Sendto(const void *buf, int len,const KAddr* addr)
	{
		return ::sendto(m_socket,(const char*)buf,len,0,addr->sockAddr(),addr->sockAddrLen());
	}

	inline int Recvfrom(void *buf, int len,KAddr *from)
	{
		sockaddr_in6 inaddr;
		int addrlen=sizeof(inaddr);
		int rslt= ::recvfrom(m_socket,(char*)buf,len,0,(sockaddr*)&inaddr,&addrlen);
		from->set((sockaddr*)&inaddr,addrlen);
		return rslt;
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


