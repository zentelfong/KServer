#pragma once
#include "KUtil.h"
#include "xxhash/xxhash.h"
#include "rc4/rc4.h"

//UDP socket的封装,添加数据加密及数据验证
#define RC4_KEY {35,197,205,237,220,202,185,240,69,43,222,51,203,103,7,172}

class KSocket
{
public:
	enum{
		XXHASH_SEED=0x0A521248,
	};

	KSocket(int af)
	{
		memset(&m_xxhash,0,sizeof(XXH32_state_t));
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
	int EncodePacket(unsigned char *buf, int len)
	{
		//对所有数据进行校验
		XXH32_reset(&m_xxhash,XXHASH_SEED);
		XXH32_update(&m_xxhash,buf,len);
		uint32_t hashVal=XXH32_digest(&m_xxhash);

		//使用rc4加密
		unsigned char key[16]=RC4_KEY;
		for (int i=0;i<4;++i)
		{
			((uint32_t*)key)[i]^=hashVal;
		}

		rc4_setup(&m_rc4,key,sizeof(key));
		rc4_crypt(&m_rc4,buf,len);
		*(uint32_t*)(buf+len)=kByteSwapLE(hashVal);
		return len+4;
	}

	//解密数据包并做完整性验证
	int DecodePacket(unsigned char *buf, int len)
	{
		if (len<4)
			return -1;
		len-=4;
		uint32_t hashVal=kByteSwapLE(*(uint32_t*)(buf+len));
		unsigned char key[16]=RC4_KEY;
		for (int i=0;i<4;++i)
		{
			((uint32_t*)key)[i]^=hashVal;
		}

		rc4_setup(&m_rc4,key,sizeof(key));
		rc4_crypt(&m_rc4,buf,len);

		XXH32_reset(&m_xxhash,XXHASH_SEED);
		XXH32_update(&m_xxhash,buf,len);
		if(XXH32_digest(&m_xxhash)==hashVal)
			return len;
		else
			return 0;
	}

	inline int SendMsg(const iovec* msg,int count,const KAddr* addr)
	{
		unsigned char data[1600];
		int dataLen=0;
		for (int i=0;i<count;++i)
		{
			memcpy(data+dataLen,msg[i].iov_base,msg[i].iov_len);
			dataLen+=msg[i].iov_len;
		}
		dataLen=EncodePacket(data,dataLen);//加密数据包
		return ::sendto(m_socket,(const char*)data,dataLen,0,addr->sockAddr(),addr->sockAddrLen());
	}

	inline int Sendto(const void *buf, int len,const KAddr* addr)
	{
		unsigned char data[1600];
		memcpy(data,buf,len);
		len=EncodePacket(data,len);//加密数据包
		return ::sendto(m_socket,(const char*)data,len,0,addr->sockAddr(),addr->sockAddrLen());
	}

	inline int Recvfrom(void *buf, int len,KAddr *from)
	{
		sockaddr_in6 inaddr;
		int addrlen=sizeof(inaddr);
		int rslt= ::recvfrom(m_socket,(char*)buf,len,0,(sockaddr*)&inaddr,&addrlen);//失败返回-1
		if (rslt>0)
		{
			rslt=DecodePacket((unsigned char *)buf,rslt);
			if (len>0)
				from->set((sockaddr*)&inaddr,addrlen);
		}
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
	rc4_state     m_rc4;
};


