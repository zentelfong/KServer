#include "KSysApi.h"

class KAddr
{
public:

	KAddr(int af=AF_INET)
	{
		memset(&m_addr,0, sizeof(m_addr));
		m_sinFamily=af;
		if (m_sinFamily==AF_INET)
		{
			m_addr.addrv4.sin_family=AF_INET;
		}
		else if (m_sinFamily==AF_INET6)
		{
			m_addr.addrv6.sin6_family=AF_INET6;
		}
	}

	KAddr(const KAddr& addr)
	{
		m_sinFamily=addr.m_sinFamily;
		m_addr=addr.m_addr;
	}

	KAddr(const sockaddr_in & addr)
	{
		m_sinFamily = AF_INET;
		m_addr.addrv4=addr;
	}

	KAddr(const sockaddr_in6 & addr)
	{
		m_sinFamily = AF_INET6;
		m_addr.addrv6=addr;
	}


	//ÉèÖÃµØÖ·
	inline bool setIp(const char* ip)
	{
		memset(&m_addr,0,sizeof(m_addr));  
		const char*p = ip;  
		int cnt = 0;  
		for(; *p != '\0';p++)
		{
			if(*p == ':')cnt++;
		}

		if(cnt >= 2)  
		{  
			m_sinFamily = AF_INET6;
			m_addr.addrv6.sin6_family=AF_INET6;
			if( inet_pton(PF_INET6,ip,&m_addr.addrv6.sin6_addr) <= 0)  
				return false;  
		}else  
		{  
			m_sinFamily = AF_INET;  
			m_addr.addrv4.sin_family=AF_INET;
			if( inet_pton(PF_INET,ip,&m_addr.addrv4.sin_addr) <= 0)  
				return false;  
		}  
		return true;  
	}

	inline void setPort(int port)
	{
		if (m_sinFamily==AF_INET)
		{
			m_addr.addrv4.sin_port=htons(port);
		}
		else if (m_sinFamily==AF_INET6)
		{
			m_addr.addrv6.sin6_port=htons(port);
		}
	}

	inline void set(const sockaddr_in & addr)
	{
		m_sinFamily = AF_INET;
		m_addr.addrv4=addr;
	}

	inline void set(const sockaddr_in6 & addr)
	{
		m_sinFamily = AF_INET6;
		m_addr.addrv6=addr;
	}


	inline void set(const sockaddr* addr,int len)
	{
		if (len==sizeof(sockaddr_in))
		{
			m_sinFamily=AF_INET;
			m_addr.addrv4=*(sockaddr_in*)addr;
		}
		else if (len==sizeof(sockaddr_in6))
		{
			m_sinFamily=AF_INET6;
			m_addr.addrv6=*(sockaddr_in6*)addr;
		}
	}

	const inline sockaddr* sockAddr() const 
	{
		return (sockaddr*)&m_addr;
	}

	inline sockaddr* sockAddr() 
	{
		return (sockaddr*)&m_addr;
	}

	const inline int sockAddrLen() const
	{
		if (m_sinFamily==AF_INET)
			return sizeof(sockaddr_in);
		else if (m_sinFamily==AF_INET6)
			return sizeof(sockaddr_in6);
		else
			return 0;
	}

	const bool isIpv6() const {return m_sinFamily == AF_INET6;}  

private:
	short int m_sinFamily; //address family AF_INET or AF_INET6
	union{
		sockaddr_in addrv4;
		sockaddr_in6 addrv6;
	}m_addr;
};