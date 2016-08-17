#ifndef _KFEC_H_
#define _KFEC_H_

#include "fec.h"
#include "KUtil.h"
#include "KEndian.h"


//默认10个数据包，产生2个冗余包，冗余20%
#define FEC_DATA_BLOCK_COUNT 10
#define FEC_ALL_BLOCK_COUNT  12
#define FEC_FEC_BLOCK_COUNT  (FEC_ALL_BLOCK_COUNT-FEC_DATA_BLOCK_COUNT)
#define FEC_MTU 1400

struct KFecPacket 
{
	uint32_t grpid:24;//组id
	uint32_t pckid:8; //包id 0~FEC_ALL_BLOCK_COUNT-1
};

//数据编码
class KFecEncode
{
public:
	KFecEncode(fec_t* fec);
	~KFecEncode();

	//写数据包
	int WritePacket(const char* data,int len);

	//将数据包用UDP发送出去
	int SendPacket(uint16_t group,uint8_t idx,const char* data,int len)
	{
		KFecPacket packet;
		packet.grpid=group;
		packet.pckid=idx;
		packet=kEndianScalar<KFecPacket>(packet);
		printf("SendPacket %d/%d\n",(int)group,(int)idx);

		return 0;
	}

private:
	uint16_t m_groupId;//当前分组id
	int m_blockCount;//已处理的数据数
	fec_t* m_fec;//fec
	char * m_fecBuffer;//冗余信息缓冲区
	char * m_fecBlocks[FEC_FEC_BLOCK_COUNT];
};



//数据解码
class KFecDecode
{
public:
	KFecDecode(fec_t* fec);
	~KFecDecode();

	enum {
		GROUP_COUNT = 16,
	};

	//收到数据调用
	void ReadPacket(const char* data,int len);

	//收到包
	int RecvPacket(const char* data,int len){}

	//发送的数据包为
	// conv fecPacket data


	struct FECGroup{
		uint16_t grpid;
		char recved[FEC_DATA_BLOCK_COUNT][FEC_MTU];//已处理的分组内的包
	};
private:
	FECGroup m_fecGroup[GROUP_COUNT];//接收的数据恢复
	fec_t* m_fec;//fec
};












#endif