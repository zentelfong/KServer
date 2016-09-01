#pragma once
#include "KUtil.h"
#include "KFEC.h"
#include "KSocket.h"

typedef IUINT32 kcp_t;

enum KEVENT_TYPE{
	KEV_NEW_CONN=0x1,
	KEV_READ=0x2,
	KEV_WRITE=0x4,
	KEV_ERR=0x8,
	KEV_TIMEOUT=0x10,
	KEV_CLOSE=0x20,
};

//心跳包等类型
enum KCONTROL_TYPE{
	KCT_NONE,
	KCT_CONNECT,//连接控制类型
	KCT_CLOSE,//关闭连接类型
	KCT_KEEP_ALIVE,//保持连接及心跳
};

struct KControlPacketHead
{
	void Read(const char* buf)
	{
		unuse=kReadScalar<uint32_t>(buf);
		controlType=kReadScalar<uint32_t>(buf+4);
		kcpid=kReadScalar<uint32_t>(buf+8);
	}

	void Write(char* buf)
	{
		kWriteScalar(buf,unuse);
		kWriteScalar(buf+4,controlType);
		kWriteScalar(buf+8,kcpid);
	}

	uint32_t unuse;//为0
	uint32_t controlType;//控制类型
	uint32_t kcpid;//连接的id
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
		timeOutInterval=60*1000;//默认1分钟
		keepAliveInterval=15*1000;//默认15s发送一次心跳
		mtu=1400;
		sndwnd=32;
		rcvwnd=32;
		interval=100;

		nodelay=false;
		fastResend=true;
		enableCC=true;

		ipv6=false;
		stream=true;

		minrto=100;
	}
	ktime_t	timeOutInterval;//超时时间间隔
	ktime_t keepAliveInterval;//心跳时间间隔
	int mtu;//MTU
	int sndwnd;//发送滑动窗口大小
	int rcvwnd;//接收滑动窗口大小
	int interval;//更新时间间隔

	bool nodelay;//是否无延迟发包
	bool fastResend;//是否开启快速重传
	bool enableCC;//是否开启流量控制
	bool ipv6;//是否使用ipv6
	bool stream;//流模式
	int  minrto; 
};



//连接
#if FEC_ENABLE
class KConnection:public KMalloc,public KFecPacketTransfer
{
public:
	KConnection(kcp_t kcp)
		:m_fecEncode(this),m_fecDecode(this)
#else
class KConnection:public KMalloc
{
public:
	KConnection(kcp_t kcp)
#endif
	{
		m_socket=NULL;
		ikcp_ctor(&m_kcp,kcp,this);
		m_kcp.output=SendPacket;
		m_kcp.writelog=OutLog;

		min_heap_idx=-1;
		m_checkTime=0;
		m_lastRecv=kTime();
		m_lastSend=m_lastRecv;
	}

	~KConnection()
	{
		ikcp_dtor(&m_kcp);
	}

	inline void SetSocket(KSocket* socket)
	{
		m_socket=socket;
	}

	//设置为流式,默认是包式
	inline void SetStreamMode(bool stream)
	{
		m_kcp.stream=stream;
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

	//0成功，-1失败
	inline int RecvPacket(const char *data, long size,ktime_t time,const KAddr* addr)
	{
		m_destAddr=*addr;//更新地址
		m_lastRecv=time;
#if FEC_ENABLE
		data+=sizeof(kcp_t);
		size-=sizeof(kcp_t);
		return m_fecDecode.DecodePacket(data,size);
#else
		return ikcp_input(&m_kcp,data,size);
#endif
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

	inline void SetMinRTO(int minrto)
	{
		m_kcp.rx_minrto=minrto;
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
	inline ktime_t GetLastSendTime(){return m_lastSend;}
	inline ktime_t GetCheckTime(){return m_checkTime;}
	inline kcp_t   GetKcpId(){return m_kcp.conv;}
	inline void    SetLastSendTime(ktime_t current){m_lastSend=current;}
	inline void    SetLastRecvTime(ktime_t current){m_lastRecv=current;}
#if FEC_ENABLE
	void SetFec(fec_t* fec)
	{
		m_fecEncode.SetFec(fec);
		m_fecDecode.SetFec(fec);
	}

	virtual int SendPacket(KFecPacket* packet)
	{
		//调用UDP发送数据包
		m_lastSend=kTime();
		iovec iov[2];
		char head[sizeof(kcp_t)+sizeof(KFecPacketHead)];
		kWriteScalar<uint32_t>(head,GetKcpId());
		kWriteScalar<KFecPacketHead>(head+sizeof(kcp_t),packet->head);

		iov[0].iov_base=head;
		iov[0].iov_len=sizeof(head);

		iov[1].iov_base=(char*)packet->data;
		iov[1].iov_len=packet->len;
		return m_socket->SendMsg(iov,2,&m_destAddr);
	}

	virtual int RecvPacket(const char* data,int len)
	{
		//fec解码的包不包含fechead
		return ikcp_input(&m_kcp,data,len);
	}
#endif

public:
	int min_heap_idx;//小根堆的索引
private:
	static int SendPacket(const char *buf, int len, struct IKCPCB *kcp, void *user)
	{
		KConnection* pThis=(KConnection*)user;
#if FEC_ENABLE
		return pThis->m_fecEncode.EncodePacket(buf,len);
#else
		return pThis->m_socket->Sendto(buf,len,&pThis->m_destAddr);
#endif
	}

	static void OutLog(const char *log, struct IKCPCB *kcp, void *user)
	{
		printf("%s\n",log);
	}

	friend class KServer;
	friend class KClient;

	ikcpcb m_kcp;
	ktime_t m_checkTime;//下一次检查时间
	ktime_t  m_lastRecv;//上次接收数据时间
	ktime_t  m_lastSend;//上次发送数据时间
	KAddr    m_destAddr;//对方地址
	KSocket* m_socket;

#if FEC_ENABLE
	KFecEncode m_fecEncode;
	KFecDecode m_fecDecode;
#endif
};
