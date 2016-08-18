#include <stdio.h>
#include "KServer.h"
#include "KClient.h"

//单位s
#define RUN_SECONDS 10

void RunServer()
{
	printf("RunServer:\n");
	KServer server(AF_INET);
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9091);
	addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(addr.sin_zero), '\0', 8);

	KAddr kaddr;
	kaddr.set(addr);
	int rslt=server.Bind(&kaddr);
	assert(rslt==0);
	KEvent ev[128];

	for (int i=0;i<RUN_SECONDS*100;++i)//10s
	{
		ktime_t time=kTime();
		rslt=server.Wait(ev,128,10);
		for (int i=0;i<rslt;++i)
		{
			char buf[1024];

			if (ev[i].event & KEV_READ)
			{
				int recv=server.Recv(ev[i].kcp,buf,sizeof(buf)-1);
				if(recv>0)
				{
					buf[recv]='\0';
					printf("server %d recv %s\n",ev[i].kcp,buf);

					if (ev[i].event & KEV_WRITE)
					{
						server.Send(ev[i].kcp,buf,sizeof(buf)-1);
					}
				}
			}
		}
	}

}

void RunClient()
{
	printf("RunClient:\n");
	KClient client(AF_INET,123);
	sockaddr_in addr2;
	addr2.sin_family = AF_INET;
	addr2.sin_port = htons(0);
	addr2.sin_addr.s_addr = INADDR_ANY;
	memset(&(addr2.sin_zero), '\0', 8);

	KAddr kaddr;
	kaddr.set(addr2);
	int rslt=client.Bind(&kaddr);
	assert(rslt==0);

	client.Connect("127.0.0.1",9091);

	const char* test="hello world!";
	for(int i=0;i<32;++i)
		client.Send(test,strlen(test)+1);
	KEvent ev[128];

	for (int i=0;i<RUN_SECONDS*100;++i)//10s
	{
		ktime_t time=kTime();
		int rslt=client.Wait(ev,128,10);

		for (int i=0;i<rslt;++i)
		{
			if (ev[i].event & KEV_READ)
			{
				char buf[1024];
				int recv=client.Recv(buf,sizeof(buf)-1);
				if(recv>0)
				{
					buf[recv]='\0';
					printf("client recv %s\n",buf);
				}
			}
		}
	}
}


#include <vector>


#define CLINET_COUNT 100

void RunMultiClient()
{
	printf("RunMultiClient:\n");

	std::vector<KClient*> clients;

	for (int i=0;i<CLINET_COUNT;++i)
	{
		KClient* client=new KClient(AF_INET,i);
		sockaddr_in addr2;
		addr2.sin_family = AF_INET;
		addr2.sin_port = htons(0);
		addr2.sin_addr.s_addr = INADDR_ANY;
		memset(&(addr2.sin_zero), '\0', 8);

		KAddr kaddr;
		kaddr.set(addr2);
		int rslt=client->Bind(&kaddr);
		assert(rslt==0);

		client->Connect("127.0.0.1",9091);
		const char* test="hello world!";
		client->Send(test,strlen(test)+1);
		clients.push_back(client);
	}


	KEvent ev[128];

	for (int i=0;i<RUN_SECONDS*100;++i)//10s
	{
		ktime_t time=kTime();

		for (int i=0;i<CLINET_COUNT;++i)
		{
			int rslt=clients[i]->Wait(ev,128,10);

			if(rslt)
			{
				if (ev[0].event & KEV_READ)
				{
					char buf[1024];
					int recv=clients[i]->Recv(buf,sizeof(buf)-1);
					if(recv>0)
					{
						buf[recv]='\0';
						printf("client %d recv %s\n",ev[0].kcp,buf);
					}
				}
			}
		}
	}
}


class FecTransport:public KFecPacketTransfer
{
public:
	FecTransport()
		:m_encode(NULL),m_decode(NULL)
	{
		send=0;
		recv=0;
	}

	virtual int SendPacket(KFecPacket* packet)
	{
		

		char buf[FEC_MTU+sizeof(KFecPacket)];
		kWriteScalar<KFecPacketHead>(buf,packet->head);
		memcpy(buf+sizeof(KFecPacketHead),packet->data,packet->len);

		//5号包和6号包丢失
		if(packet->head.pckid!=5 && packet->head.pckid!=0)
		{
			printf("send packet %d,%d\n",packet->head.grpid,packet->head.pckid);
			return m_decode->DecodePacket(buf,packet->len+sizeof(KFecPacketHead))+sizeof(KFecPacketHead);
		}
		else
		{
			printf("丢失了包 %d,%d\n",packet->head.grpid,packet->head.pckid);
			return packet->len+sizeof(KFecPacketHead);
		}
	}

	//不含kfechead
	virtual int RecvPacket(const char* data,int len)
	{
		printf("recv packet %d %s\n",recv++,data);
		//return m_encode->EncodePacket(data,len);
		return len;
	}

	int send;
	int recv;
	KFecEncode* m_encode;
	KFecDecode* m_decode;
};


void TestFEC()
{
	int len=sizeof(KFecPacketHead);

	fec_t* fec= fec_new(FEC_DATA_BLOCK_COUNT,FEC_ALL_BLOCK_COUNT);

	FecTransport transport;
	KFecEncode encode(&transport);
	
	KFecDecode decode(&transport);
	encode.SetFec(fec);
	decode.SetFec(fec);
	transport.m_encode=&encode;
	transport.m_decode=&decode;

	for (int i=0;i<20;++i)
	{
		char buf[]="123456789123456789";
		buf[0]='a'+i;
		encode.EncodePacket(buf,strlen(buf)+1);
	}
	
	fec_delete(fec);
}

void TestMalloc()
{
#define TEST_COUNT 1000000

	DWORD tick=GetTickCount();
	KMemPoll* poll=KThreadMemPoll::Current();
	for (int i=0;i<TEST_COUNT;++i)
	{
		void* mem=poll->Malloc(i%2048);
		poll->Free(mem);
	}
	printf("KMemPoll耗时 %d\n",GetTickCount()-tick);

	tick=GetTickCount();
	for (int i=0;i<TEST_COUNT;++i)
	{
		void* mem=kMalloc(i%2048);
		kFree(mem);
	}
	printf("kmalloc耗时 %d\n",GetTickCount()-tick);

	tick=GetTickCount();
	for (int i=0;i<TEST_COUNT;++i)
	{
		void* mem=malloc(i%2048);
		free(mem);
	}
	printf("malloc耗时 %d\n",GetTickCount()-tick);

/*
win7 i7 3.4GHZ测试结果如下：
KMemPoll耗时 265
kmalloc耗时 390
malloc耗时 1732	
*/
}

int main()
{
	KThreadMemPoll poll(1024*1024);
	KNetInitialized netinit;
	//TestFEC();
	//TestMalloc();

	printf("输入s启动服务器，c启动客户端,m启动多个客户端\n");

	int ch=getchar();
	if (ch=='s')
	{
		RunServer();
	}
	else if (ch=='c')
	{
		RunClient();
	}
	else if (ch=='m')
	{
		RunMultiClient();
	}

	printf("AllocCount %d\n",poll.GetAllocCount());
	system("pause");
	return 0;
}
