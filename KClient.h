#pragma once

#include "KUtil.h"
#include "KConnection.h"
#include "KSocket.h"


class KClient:public KTransportBase
{
public:
	KClient(kcp_t kcpId)
		:m_connection(this,kcpId)
	{
		ikcp_allocator(kMalloc,kFree);
	}

	int Bind(const sockaddr* addr){return m_socket.Bind(addr);}

	void Connect(const char* ip,int port)
	{
		KAddr addr;
		addr.set(ip,port);
		m_connection.SetAddr(addr.sockAddr());
		m_connection.SetMTU(m_options.mtu);
		m_connection.SetWndSize(m_options.sndwnd,m_options.rcvwnd);
		m_connection.SetNodelay(m_options.nodelay,m_options.updateInterval,m_options.fastResend,m_options.enableCC);
	}

	int Send(const char* data,int len)
	{
		int rslt=m_connection.Send(data,len);
		m_connection.m_checkTime=0;
		return rslt;
	}

	int Recv(char* data,int len)
	{
		int rslt=m_connection.Recv(data,len);
		return rslt;
	}

	int Wait(KEvent *ev,int evMax,ktime_t delay=0);

	void SetOptions(const KOptions* opt){m_options=*opt;}
protected:

	virtual int SendPacket(const struct sockaddr* addr,const char *buf, int len)
	{
		return m_socket.Sendto(buf,len,addr);
	}

	virtual void OutLog(const char *log)
	{
		printf("%s",log);
	}

private:
	KSocket m_socket;
	KConnection m_connection;
	KOptions m_options;
};