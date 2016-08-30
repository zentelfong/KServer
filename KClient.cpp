#include "KClient.h"


int KClient::Wait(KEvent *ev,int evMax,ktime_t delay)
{
	int count=0;

	ktime_t time=kTime();

	char buf[1600];
	KAddr addr;

	int recv=0;

	if(m_socket.CheckReadable(delay))
	{
		do 
		{
			recv=m_socket.Recvfrom(buf,sizeof(buf),&addr);
			if (recv>4)
			{
				kcp_t kcpid=0;
#if IWORDS_BIG_ENDIAN
				kcpid = *(const unsigned char*)(buf + 3);
				kcpid = *(const unsigned char*)(buf + 2) + (kcpid << 8);
				kcpid = *(const unsigned char*)(buf + 1) + (kcpid << 8);
				kcpid = *(const unsigned char*)(buf + 0) + (kcpid << 8);
#else 
				kcpid = *(const kcp_t*)buf;
#endif
				if (kcpid==m_connection.GetKcpId())
				{
					m_connection.RecvPacket(buf,recv,time,&addr);
				}
			}
			else
				break;
		} while (1);
	}

	if (time - m_connection.GetLastRecvTime() > m_options.timeOutInterval)
	{
		if(count<evMax)
		{
			ev[count].kcp=m_connection.GetKcpId();
			ev[count].event=KEV_TIMEOUT;
			++count;
		}
	}
	else if (time - m_connection.GetLastSendTime() > m_options.keepAliveInterval)
	{
		m_connection.SetLastSendTime(time);
		char buf[24]={0};
		KControlPacketHead keepAlivePacket;
		keepAlivePacket.unuse=0;
		keepAlivePacket.kcpid=m_connection.GetKcpId();
		keepAlivePacket.controlType=KCT_KEEP_ALIVE;
		keepAlivePacket.Write(buf);
		m_socket.Sendto(buf,sizeof(buf),&m_connection.m_destAddr);
	}
	else
	{
		m_connection.Update(time);
		if(count<evMax)
		{
			ev[count].kcp=m_connection.GetKcpId();
			ev[count].event=0;
			int readable=0,writeable=0;

			if(m_connection.CheckReadWrite(&readable,&writeable))
			{
				if(readable)
					ev[count].event |= KEV_READ;
				if(writeable)
					ev[count].event |= KEV_WRITE;
				++count;
			}
		}
	}
	return count;
}