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
				kcp_t kcpid=kReadScalar<kcp_t>(buf);
				if (kcpid>0)//如果收到kcpid为0表示客户端的keepalive
				{
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
						conn->SetSocket(&m_socket);
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
				{
					//KServer的控制数据包,keepAlive，connect等
					KControlPacketHead packetHead;
					packetHead.Read(buf);
					switch (packetHead.controlType)
					{
					case KCT_KEEP_ALIVE:
						{
							KConnection * conn=m_kcpHash.Find(kcpid);
							if (conn)
							{
								printf("keepAlive %d \n",packetHead.kcpid);
								conn->SetLastRecvTime(time);

								char buf[24]={0};
								KControlPacketHead keepAlivePacket;
								keepAlivePacket.unuse=0;
								keepAlivePacket.kcpid=conn->GetKcpId();
								keepAlivePacket.controlType=KCT_KEEP_ALIVE;
								keepAlivePacket.Write(buf);
								m_socket.Sendto(buf,sizeof(buf),&conn->m_destAddr);
							}
							break;
						}

					case KCT_CONNECT:

						break;
					case KCT_CLOSE:
						{
							KConnection * conn=m_kcpHash.Find(kcpid);
							if (conn)
							{
								m_kcpHeap.Erase(conn);
								m_kcpHash.Remove(kcpid);
								delete conn;

								if(count<evMax)
								{
									ev[count].kcp=kcpid;
									ev[count].event=KEV_CLOSE;
									++count;
								}
							}
							break;
						}
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

		if (time - conn->GetLastRecvTime() > m_options.timeOutInterval)//3分钟超时
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


