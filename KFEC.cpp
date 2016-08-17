#include "KFEC.h"
#include <string.h> //for memset,memcpy
#include <malloc.h>


KFecEncode::KFecEncode(fec_t* fec)
	:m_groupId(0),m_blockCount(0),m_fec(fec)
{
	m_fecBuffer=(char*)kMalloc(FEC_MTU*FEC_FEC_BLOCK_COUNT);
	memset(m_fecBuffer,0,FEC_MTU*FEC_FEC_BLOCK_COUNT);
	for (int i=0;i<FEC_FEC_BLOCK_COUNT;++i)
	{
		m_fecBlocks[i]=m_fecBuffer+i*FEC_MTU;
	}
}

KFecEncode::~KFecEncode()
{
	fec_delete(m_fec);
	kFree(m_fecBuffer);
}


int KFecEncode::WritePacket(const char* data,int len)
{
	gf* src[1];
	src[0]=(unsigned char*)data;

	if (len<FEC_MTU)
	{
		char* tmp=(char*)alloca(FEC_MTU);
		memcpy(tmp,data,len);
		memset(tmp+len,0,FEC_MTU-len);
		data=tmp;
	}

	//fec_encode(m_fec,src,(gf**)m_fecBlocks,1,len);
	int rslt=SendPacket(m_groupId,m_blockCount++,data,FEC_MTU);

	if (m_blockCount>=FEC_DATA_BLOCK_COUNT)
	{
		//发出去后，重置缓冲区
		SendPacket(m_groupId,m_blockCount++,m_fecBlocks[0],FEC_MTU);
		SendPacket(m_groupId,m_blockCount++,m_fecBlocks[1],FEC_MTU);

		memset(m_fecBuffer,0,FEC_MTU*FEC_FEC_BLOCK_COUNT);
		++m_groupId;
		m_blockCount=0;
	}

	return rslt;
}



















KFecDecode::KFecDecode(fec_t* fec)
	:m_fec(fec)
{
	memset(m_fecGroup,0,sizeof(m_fecGroup));
}

KFecDecode::~KFecDecode()
{

}

void KFecDecode::ReadPacket(const char* data,int len)
{
	KFecPacket packet;
	packet=kReadScalar<KFecPacket>(data);
	if (packet.pckid<=FEC_ALL_BLOCK_COUNT)
	{
		FECGroup& group=m_fecGroup[packet.grpid%GROUP_COUNT];
		if (packet.grpid > group.grpid)
		{
			group.grpid=packet.grpid;

		}
		else if(packet.grpid != group.grpid && packet.pckid>=FEC_DATA_BLOCK_COUNT)
		{
			//过时的冗余包不处理
			return;
		}


	}
	else
	{
		printf("packid %d 不合法",(int)packet.pckid);
	}



}

















