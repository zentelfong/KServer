#pragma once
#include "KUtil.h"

typedef IUINT32 kcp_t;


class KTransportBase
{
public:
	virtual int  SendPacket(const struct sockaddr* addr,const char *buf, int len)=0;
	virtual void OutLog(const char *log)=0;
};


enum KEVENT_TYPE{
	KEV_NEW_CONN=0x1,
	KEV_READ=0x2,
	KEV_WRITE=0x4,
	KEV_ERR=0x8,
	KEV_TIMEOUT=0x10,
};

//类似于epoll
struct KEvent
{
	unsigned int event;
	kcp_t        kcp;
};

//连接
class KConnection:public KMalloc
{
public:
	KConnection(KTransportBase* transport,kcp_t kcp)
	{
		m_transPort=transport;
		ikcp_ctor(&m_kcp,kcp,this);
		m_kcp.output=SendPacket;
		m_kcp.writelog=OutLog;

		min_heap_idx=-1;
		m_checkTime=0;
		m_lastRecv=kTime();
		memset(&m_destAddr,0,sizeof(m_destAddr));
	}

	~KConnection()
	{
		ikcp_dtor(&m_kcp);
	}

	//更新kcp，返回下次更新时间
	inline ktime_t Update(ktime_t current)
	{
		ikcp_update(&m_kcp,current);
		m_checkTime= ikcp_check(&m_kcp,current);
		return m_checkTime;
	}

	inline int Send(const char *buffer, int len)
	{
		return ikcp_send(&m_kcp,buffer,len);
	}

	inline int Recv(char *buffer, int len)
	{
		return ikcp_recv(&m_kcp,buffer,len);
	}

	inline int RecvPacket(const char *data, long size,ktime_t time,const struct sockaddr* addr)
	{
		m_destAddr=*addr;//更新地址
		m_lastRecv=time;
		return ikcp_input(&m_kcp,data,size);
	}

	inline void Flush(){ikcp_flush(&m_kcp);}

	inline int PeekSize(){return ikcp_peeksize(&m_kcp);}


	inline int SetMTU(int mtu)
	{
		return ikcp_setmtu(&m_kcp,mtu);
	}

	inline int SetWndSize(int sndwnd, int rcvwnd)
	{
		return ikcp_wndsize(&m_kcp,sndwnd,rcvwnd);
	}

	inline int WaitSend()
	{
		return ikcp_waitsnd(&m_kcp);
	}

	inline int SetNodelay(int nodelay, int interval, int resend, int nc)
	{
		return ikcp_nodelay(&m_kcp,nodelay,interval,resend,nc);
	}

	inline int CheckReadWrite(int *readable,int *writeable)
	{
		return ikcp_check_read_write(&m_kcp,readable,writeable);
	}

	inline void SetAddr(const sockaddr* addr)
	{
		m_destAddr=*addr;
	}

	inline bool operator > (const KConnection& conn)
	{
		return m_checkTime>conn.m_checkTime;
	}

	inline ktime_t GetLastRecvTime(){return m_lastRecv;}
	inline ktime_t GetCheckTime(){return m_checkTime;}
	inline kcp_t   GetKcpId(){return m_kcp.conv;}

public:
	int min_heap_idx;//小根堆的索引
private:
	static int SendPacket(const char *buf, int len, struct IKCPCB *kcp, void *user)
	{
		KConnection* pThis=(KConnection*)user;
		return pThis->m_transPort->SendPacket(&pThis->m_destAddr,buf,len);
	}

	static void OutLog(const char *log, struct IKCPCB *kcp, void *user)
	{
		KConnection* pThis=(KConnection*)user;
		pThis->m_transPort->OutLog(log);
	}

	friend class KServer;
	friend class KClient;

	ikcpcb m_kcp;
	ktime_t m_checkTime;//下一次检查时间
	ktime_t  m_lastRecv;//上次接收数据时间
	struct sockaddr m_destAddr;//对方地址
	KTransportBase* m_transPort;
};
