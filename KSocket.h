#pragma once
#include "KUtil.h"
#include "xxhash/xxhash.h"


//UDP socket的封装,添加数据加密及数据验证
class KSocket
{
public:
	enum{
		XXHASH_SEED=0x0A521248,
		ENCRYPT_SALT=0x242F29388D239D58
	};

	KSocket(int af)
	{
		memset(&XXH32_state_t,0,sizeof(XXH32_state_t));
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

	//加密数据包
	int EncodePacket(char *buf, int len)
	{
		//对所有数据进行校验
		XXH32_reset(&m_xxhash,XXHASH_SEED);
		XXH32_update(&m_xxhash,buf,len);
		uint32_t hashVal=XXH32_digest(&m_xxhash);
		uint64_t key= hashVal * ENCRYPT_SALT + hashVal;
		//对data部分进行加密
		char* data=buf;
		int fLen=len/8*8;//fLen为8的倍数

		while (data<=fLen)
		{
			*(uint64_t*)data^=key;
			data+=8;
		}

		//尾部少于8字节
		char key2[8];
		*(uint64_t*)key2=ENCRYPT_SALT;
		for(int i=fLen;i<len;++i)
		{
			buf[fLen]^=key2[i-fLen];
		}


	}


	/////////////////////////////////////////////////////////////////////////
	//            |                  |            |                         |
	//   trnsid   |      data        |    xhash 4 |           end  4        |
	//            |                  |            |                         |
	/////////////////////////////////////////////////////////////////////////

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
		res = ::WSASendTo(m_socket, (LPWSABUF)msg, count, &size, 0, addr->sockAddr(),addr->sockAddrLen(), NULL, NULL);
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
	XXH32_state_t m_xxhash;
};


