#pragma once

#include "KUtil.h"
#include "KConnection.h"
#include "KSocket.h"


class KClient
{
public:
	KClient(int af,kcp_t kcpId)
		:m_socket(af),m_connection(kcpId)
	{
		m_connection.SetSocket(&m_socket);
#if FEC_ENABLE
		m_fec=fec_new(FEC_DATA_BLOCK_COUNT,FEC_ALL_BLOCK_COUNT);
		m_connection.SetFec(m_fec);
#endif
	}

	~KClient()
	{
#if FEC_ENABLE
		fec_delete(m_fec);
#endif
	}

	int Bind(const KAddr* addr){return m_socket.Bind(addr);}

	void Connect(const char* ip,int port)
	{
		KAddr addr;
		addr.setIp(ip);
		addr.setPort(port);
		m_connection.SetAddr(&addr);
		m_connection.SetStreamMode(m_options.stream);
		m_connection.SetMTU(m_options.mtu);
		m_connection.SetWndSize(m_options.sndwnd,m_options.rcvwnd);
		m_connection.SetNodelay(m_options.nodelay,m_options.interval,m_options.fastResend,m_options.enableCC);
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

	//调用connect前调用有效
	void SetOptions(const KOptions* opt){m_options=*opt;}

private:
	KSocket m_socket;
	KConnection m_connection;
	KOptions m_options;
#if FEC_ENABLE
	fec_t * m_fec;
#endif
};