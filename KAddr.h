#include "KSysApi.h"

class KAddr:public sockaddr_in
{
public:
	KAddr()
	{
		memset(sin_zero, '\0', sizeof(sin_zero));
		sin_family = AF_INET;
		sin_port=0;
		sin_addr.s_addr=0;
	}

	KAddr(const char* ip,int port)
	{
		memset(sin_zero, '\0', sizeof(sin_zero));
		sin_family = AF_INET;
		sin_port=(u_short)htons(port);
		sin_addr.s_addr=inet_addr(ip);
	}

	KAddr(const KAddr& addr)
	{
		memset(sin_zero, '\0', sizeof(sin_zero));
		sin_family = AF_INET;
		sin_port=addr.sin_port;
		sin_addr.s_addr=addr.sin_addr.s_addr;
	}

	KAddr(const sockaddr_in & addr)
	{
		memset(sin_zero, '\0', sizeof(sin_zero));
		sin_family = AF_INET;
		sin_port=addr.sin_port;
		sin_addr.s_addr=addr.sin_addr.s_addr;
	}

	//…Ë÷√µÿ÷∑
	inline void set(const char* ip,int port)
	{
		sin_port=htons(port);
		sin_addr.s_addr=inet_addr(ip);
	}

	inline void set(const sockaddr_in& addr)
	{
		sin_port=addr.sin_port;
		sin_addr.s_addr=addr.sin_addr.s_addr;
	}

	inline const char* ip()
	{
		return inet_ntoa(sin_addr);
	}

	inline int port()
	{
		return ntohs(sin_port);
	}

	inline sockaddr* sockAddr(){return (sockaddr*)this;}
};