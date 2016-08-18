#include "KServer.h"

int KServer::Wait(KEvent *ev,int evMax,int delay)
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
				KConnection * conn=m_kcpHash.Find(kcpid);
				if (conn)
				{
					conn->RecvPacket(buf,recv,time,&addr);
					//下一个周期更新
					m_kcpHeap.Erase(conn);
					conn->m_checkTime=0;
					m_kcpHeap.Push(conn);
				}
				else
				{
					conn=new KConnection(kcpid);//新连接到来
#if FEC_ENABLE
					conn->SetFec(m_fec);
#endif
					conn->SetStreamMode(m_options.stream);
					conn->SetTransport(this);
					conn->SetMTU(m_options.mtu);
					conn->SetWndSize(m_options.sndwnd,m_options.rcvwnd);
					conn->SetNodelay(m_options.nodelay,m_options.interval,m_options.fastResend,m_options.enableCC);

					m_kcpHash.Set(kcpid,conn);
					m_kcpHeap.Push(conn);
					conn->RecvPacket(buf,recv,time,&addr);

					printf("new connection %d\n",kcpid);

					//添加到event中
					if(count<evMax)
					{
						ev[count].kcp=kcpid;
						ev[count].event=KEV_NEW_CONN;
						++count;
					}
				}
			}
			else
				break;
		} while (1);
	}

	KConnection* conn=m_kcpHeap.Top();
	while(conn && time>=conn->GetCheckTime())
	{
		m_kcpHeap.Pop();
		kcp_t kcpId=conn->GetKcpId();

		if (time - conn->GetLastRecvTime() > m_options.connectionLife)//3分钟超时
		{
			m_kcpHash.Remove(kcpId);
			delete conn;
			printf("close connection %d\n",kcpId);
			if(count<evMax)
			{
				ev[count].kcp=kcpId;
				ev[count].event=KEV_TIMEOUT;
				++count;
			}
			else
			{
				break;
			}
		}
		else
		{
			ktime_t check=conn->GetCheckTime();
			bool bAdded=false;

			//更新后放到堆里面
			conn->Update(time);

			//检查是否可读可写
			if (check==0)
			{
				//新连接已添加过列表所以遍历查找下
				for (int i=0;i<count;++i)
				{
					if (ev[i].kcp==conn->GetKcpId())
					{
						bAdded=true;
						int readable=0,writeable=0;

						if(conn->CheckReadWrite(&readable,&writeable))
						{
							if(readable)
								ev[i].event |= KEV_READ;
							if(writeable)
								ev[i].event |= KEV_WRITE;
						}
						break;
					}
				}
			}
			
			if (!bAdded)
			{
				if(count<evMax)
				{
					ev[count].kcp=conn->GetKcpId();
					ev[count].event=0;
					int readable=0,writeable=0;

					if(conn->CheckReadWrite(&readable,&writeable))
					{
						if(readable)
							ev[count].event |= KEV_READ;
						if(writeable)
							ev[count].event |= KEV_WRITE;
						++count;
					}
				}
				else
					break;
			}
			m_kcpHeap.Push(conn);
		}
		conn=m_kcpHeap.Top();
	}
	return count;
}


