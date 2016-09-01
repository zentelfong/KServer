#include "KFEC.h"
#include <string.h> //for memset,memcpy
#include <malloc.h>


KFecEncode::KFecEncode(KFecPacketTransfer* transfer)
	:m_groupId(0),m_packetId(0),m_transfer(transfer),m_fec(NULL)
{
	memset(m_buffer,0,sizeof(m_buffer));
}

KFecEncode::~KFecEncode()
{
	ReleaseBuffer();
}

void KFecEncode::ReleaseBuffer()
{
	for (int i=0;i<FEC_DATA_BLOCK_COUNT;++i)
	{
		kFree((char*)m_buffer[i].data);
		m_buffer[i].data=NULL;
		m_buffer[i].len=0;
	}
}

int KFecEncode::EncodePacket(const char* data,int len)
{
	fec_enc_data& buf=m_buffer[m_packetId];
	buf.data=(gf*)kRealloc((gf*)buf.data,len+1);
	memcpy((void*)buf.data,data,len);
	buf.data[len]=FEC_PACKET_END;
	buf.len=len+1;

	KFecPacket packet;
	packet.head.grpid=m_groupId;
	packet.head.pckid=m_packetId;
	packet.data=(const char*)buf.data;
	packet.len=buf.len;

	int rslt=m_transfer->SendPacket(&packet);
	if (rslt==sizeof(KFecPacketHead)+packet.len)
	{
		if(++m_packetId==FEC_DATA_BLOCK_COUNT)
		{
			//生成冗余包，发送出去
			size_t blockSize=0;

			//冗余数据块的长度为最大的那一个数据包的长度
			for (int i=0;i<FEC_DATA_BLOCK_COUNT;++i)
			{
				if (m_buffer[i].len>blockSize)
					blockSize=m_buffer[i].len;
			}

			gf* fec[FEC_FEC_BLOCK_COUNT];
			for (int i=0;i<FEC_FEC_BLOCK_COUNT;++i)
			{
				fec[i]=(gf*)kMalloc(blockSize);//申请冗余数据包
			}

			//计算冗余数据
			fec_encode2(m_fec,m_buffer,FEC_DATA_BLOCK_COUNT,fec,FEC_FEC_BLOCK_COUNT,blockSize);

			//发送冗余数据包
			for (int i=0;i<FEC_FEC_BLOCK_COUNT;++i)
			{
				packet.head.pckid=i+FEC_DATA_BLOCK_COUNT;
				packet.data=(char*)fec[i];
				packet.len=blockSize;
				m_transfer->SendPacket(&packet);
				kFree(fec[i]);//释放冗余数据包
			}

			ReleaseBuffer();//释放缓存的buffer

			//使用下一个分组id
			m_packetId=0;
			++m_groupId;
		}
	}
	return rslt;
}



KFecDecode::KFecDecode(KFecPacketTransfer* transfer)
	:m_transfer(transfer),m_fec(NULL)
{
	memset(m_fecGroup,0,sizeof(m_fecGroup));
}

KFecDecode::~KFecDecode()
{
	for (int i=0;i<GROUP_COUNT;++i)
	{
		ClearGroup(&m_fecGroup[i]);
	}
}

int KFecDecode::DecodePacket(const char* data,int len)
{
	int rslt=-1;
	KFecPacketHead packet;
	packet=kReadScalar<KFecPacketHead>(data);

	if (packet.pckid<FEC_ALL_BLOCK_COUNT && len> FEC_OVERHEAD)
	{
		//去掉fec头部数据
		data=data+sizeof(KFecPacketHead);
		len=len-sizeof(KFecPacketHead);

		if(packet.pckid<FEC_DATA_BLOCK_COUNT)
			rslt=m_transfer->RecvPacket(data,len-1);//收到数据包

		FECGroup& group=m_fecGroup[packet.grpid%GROUP_COUNT];
		fec_dec_data& fecData=group.data[packet.pckid];


		if (packet.grpid==group.grpid)
		{
			if (!group.resumed)
			{
				//检查包数释放完整，如果完整则进行恢复过程
				fecData.data=(gf*)kRealloc((gf*)fecData.data,len);
				memcpy((gf*)fecData.data,data,len);
				fecData.len=len;
				fecData.idx=packet.pckid;
				CheckGroup(&group);
			}
			//else
			//已被恢复，过滤
		}
		else if (packet.grpid > group.grpid)
		{
			//写到该group中
			ClearGroup(&group);
			group.grpid=packet.grpid;
			fecData.data=(gf*)kRealloc((gf*)fecData.data,len);
			memcpy((gf*)fecData.data,data,len);
			fecData.len=len;
			fecData.idx=packet.pckid;
		}
		else if(packet.grpid != group.grpid && packet.pckid>=FEC_DATA_BLOCK_COUNT)
		{
			//过时的冗余包不处理
			return -1;
		}

		return rslt+1;
	}
	else
	{
		printf("packid %d 不合法",(int)packet.pckid);
		return -1;
	}
}



void KFecDecode::ClearGroup(FECGroup* group)
{
	for (int i=0;i<FEC_ALL_BLOCK_COUNT;++i)
	{
		kFree((gf*)group->data[i].data);
	}
	memset(group,0,sizeof(FECGroup));
}

void KFecDecode::FinishGroup(FECGroup* group)
{
	for (int i=0;i<FEC_ALL_BLOCK_COUNT;++i)
	{
		kFree((gf*)group->data[i].data);
	}
	memset(group->data,0,sizeof(group->data));
	group->resumed=true;
}

//检查是否可以还原丢失的包
void KFecDecode::CheckGroup(FECGroup* group)
{
	int dataCount=0;
	int fecCount=0;
	int fecIndex=FEC_DATA_BLOCK_COUNT;
	size_t blockSize=0;
	fec_dec_data data[FEC_ALL_BLOCK_COUNT]={0};//已接收的，供恢复的数据

	for (int i=0;i<FEC_ALL_BLOCK_COUNT;++i)
	{
		if (group->data[i].data)
		{
			if(i<FEC_DATA_BLOCK_COUNT)
				data[i]=group->data[i];
			if(group->data[i].idx>=FEC_DATA_BLOCK_COUNT)
				++fecCount;
			else
				++dataCount;
		}
		else if(i<FEC_DATA_BLOCK_COUNT)
		{
			//数据包不在，取冗余包来填补
			for (int j=fecIndex;j<FEC_ALL_BLOCK_COUNT;++j)
			{
				if(group->data[j].data)
				{
					data[i]=group->data[j];
					fecIndex=j+1;
					if(data[i].len>blockSize)
						blockSize=data[i].len;
					break;
				}
			}
		}
	}

	if (dataCount==FEC_DATA_BLOCK_COUNT)
	{
		//收到完整的数据包
		FinishGroup(group);
	}
	else if(dataCount+fecCount >= FEC_DATA_BLOCK_COUNT)
	{
		//使用冗余数据进行恢复
		gf* outBlock[FEC_FEC_BLOCK_COUNT];
		for (int i=0;i<FEC_FEC_BLOCK_COUNT;++i)
		{
			outBlock[i]=(gf*)kMalloc(blockSize);//申请冗余数据包
		}

		fec_decode2(m_fec,data,outBlock,blockSize);

#ifdef DEBUG
		for (int i=0;i<FEC_DATA_BLOCK_COUNT;++i)
		{
			if (group->data[i].data==NULL)
			{
				printf("恢复了包 %d/%d\n",group->grpid,i);
			}
		}
#endif

		for (int i=0;i<FEC_FEC_BLOCK_COUNT;++i)
		{
			//恢复了数据块
			int len=blockSize;
			for (int j=blockSize-1;j>=0;--j)
			{
				if (outBlock[i][j]==FEC_PACKET_END)
				{
					len=j;
					break;
				}
			}
			m_transfer->RecvPacket((char*)outBlock[i],len);
			kFree(outBlock[i]);
		}
		FinishGroup(group);
	}
}









