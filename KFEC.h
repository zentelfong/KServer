#ifndef _KFEC_H_
#define _KFEC_H_

#include "fec/fec.h"
#include "KUtil.h"
#include "KEndian.h"


//默认8个数据包，产生2个冗余包，冗余25%
//如果网络每10个包丢2个包，可以还原为不丢包
#define FEC_DATA_BLOCK_COUNT 8
#define FEC_ALL_BLOCK_COUNT  10
#define FEC_FEC_BLOCK_COUNT  (FEC_ALL_BLOCK_COUNT-FEC_DATA_BLOCK_COUNT)
#define FEC_MTU 1400

//定义为0表示不启用fec
#define FEC_ENABLE 1

//8个字节
struct KFecPacketHead
{
	uint32_t conv;    //会话id
	uint32_t grpid:24;//组id
	uint32_t pckid:8; //包id 0~FEC_ALL_BLOCK_COUNT-1
};

struct KFecPacket 
{
	KFecPacketHead head;//fec 头部4个字节
	uint32_t len;		//data 的字节数
	const char* data;
};


class KFecPacketTransfer
{
public:
	virtual int SendPacket(KFecPacket* packet)=0;
	virtual int RecvPacket(const char* data,int len)=0;
};


//数据FEC编码
class KFecEncode
{
public:
	KFecEncode(KFecPacketTransfer* transfer);
	~KFecEncode();

	//写数据包不含KFecPacketHead
	int EncodePacket(const char* data,int len);
	void SetMtu(int mtu){m_mtu=mtu;}
	void SetFec(fec_t* fec){m_fec=fec;}
private:
	void ReleaseBuffer();

	uint32_t m_groupId;//当前分组id
	uint32_t m_packetId;//当前包的id

	KFecPacketTransfer* m_transfer;
	fec_t* m_fec;//fec
	fec_enc_data m_buffer[FEC_DATA_BLOCK_COUNT];
	int m_mtu;//默认大小为FEC_MTU
};


//数据FEC解码
class KFecDecode
{
public:
	KFecDecode(KFecPacketTransfer* transfer);
	~KFecDecode();

	enum {
		GROUP_COUNT = 16,
	};

	//收到数据调用含KFecPacketHead
	int DecodePacket(const char* data,int len);
	void SetMtu(int mtu){m_mtu=mtu;}
	void SetFec(fec_t* fec){m_fec=fec;}
	struct FECGroup{
		uint16_t grpid;
		fec_dec_data data[FEC_ALL_BLOCK_COUNT];//已接收的，供恢复的数据
		bool resumed;//是否已经恢复完毕，过滤恢复完毕后，已恢复的包传过来了
	};
private:
	void ClearGroup(FECGroup* group);//清除group里面的数据缓存
	void CheckGroup(FECGroup* group);//检查group的data能否进行恢复，如果可以则进行恢复
	void FinishGroup(FECGroup* group);//已恢复该组，清除数据
	FECGroup m_fecGroup[GROUP_COUNT];//接收的数据恢复
	KFecPacketTransfer* m_transfer;//transfer
	fec_t* m_fec;//fec
	int m_mtu;//默认大小为FEC_MTU
};












#endif