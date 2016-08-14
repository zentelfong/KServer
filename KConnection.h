#pragma once
#include "KUtil.h"

typedef IUINT32 kcp_t;


class KTransportBase
{
public:
	virtual int  SendPacket(const KAddr* addr,const char *buf, int len)=0;
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

struct KOptions{
	KOptions()
	{
		connectionLife=60*1000;//默认1分钟
		mtu=1400;
		sndwnd=32;
		rcvwnd=32;
		interval=100;

		nodelay=false;
		fastResend=true;
		enableCC=true;

		ipv6=false;
	}
	ktime_t	connectionLife;//连接的生命时间长度单位毫秒
	int mtu;//MTU
	int sndwnd;//发送滑动窗口大小
	int rcvwnd;//接收滑动窗口大小
	int interval;//更新时间间隔

	bool nodelay;//是否无延迟发包
	bool fastResend;//是否开启快速重传
	bool enableCC;//是否开启流量控制
	bool ipv6;//是否使用ipv6
};


//连接
class KConnection:public KMalloc
{
public:
	KConnection(kcp_t kcp)
	{
		m_transPort=NULL;
		ikcp_ctor(&m_kcp,kcp,this);
		m_kcp.output=SendPacket;
		m_kcp.writelog=OutLog;

		min_heap_idx=-1;
		m_checkTime=0;
		m_lastRecv=kTime();
	}

	~KConnection()
	{
		ikcp_dtor(&m_kcp);
	}

	inline void SetTransport(KTransportBase* transport)
	{
		m_transPort=transport;
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

	inline int RecvPacket(const char *data, long size,ktime_t time,const KAddr* addr)
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

	inline void SetAddr(const KAddr* addr)
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
	KAddr m_destAddr;//对方地址
	KTransportBase* m_transPort;
};
