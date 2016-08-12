#include <stdio.h>
#include "KServer.h"
#include "KClient.h"


void RunServer()
{
	printf("RunServer:\n");
	KServer server;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9091);
	addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(addr.sin_zero), '\0', 8);

	int rslt=server.Bind((sockaddr*)&addr);
	assert(rslt==0);
	KEvent ev[128];

	while(1)
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
	KClient client(123);
	sockaddr_in addr2;
	addr2.sin_family = AF_INET;
	addr2.sin_port = htons(0);
	addr2.sin_addr.s_addr = INADDR_ANY;
	memset(&(addr2.sin_zero), '\0', 8);

	int rslt=client.Bind((sockaddr*)&addr2);
	assert(rslt==0);

	client.Connect("127.0.0.1",9091);

	const char* test="hello world!";
	client.Send(test,strlen(test)+1);
	KEvent ev[128];

	while(1)
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


#define CLINET_COUNT 1000

void RunMultiClient()
{
	printf("RunMultiClient:\n");

	std::vector<KClient*> clients;

	for (int i=0;i<CLINET_COUNT;++i)
	{
		KClient* client=new KClient(i);
		sockaddr_in addr2;
		addr2.sin_family = AF_INET;
		addr2.sin_port = htons(0);
		addr2.sin_addr.s_addr = INADDR_ANY;
		memset(&(addr2.sin_zero), '\0', 8);

		int rslt=client->Bind((sockaddr*)&addr2);
		assert(rslt==0);

		client->Connect("127.0.0.1",9091);
		const char* test="hello world!";
		client->Send(test,strlen(test)+1);
		clients.push_back(client);
	}


	KEvent ev[128];

	while(1)
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


int main()
{
	KNetInitialized netinit;
	int rslt=0;
	KMemPoll poll(1024*1024);
	poll.SetCurrent();

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
	getchar();
	return 0;
}
