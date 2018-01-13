#ifndef __DFU_H_
#define __DFU_H_
#include "dsEmulCmd.h"
#if 0
#define DFU_ID_LEN		16
#define DFU_HDPKT_LEN	128
#define DFU_FWTAG_LEN	32
typedef union _DFUHD_PACKET {
	struct {
		uint8_t dfuFwID[DFU_ID_LEN];//更新版本的ID,以0结尾的字符，用于区分(boot,fw,font)
		uint8_t ver[4];				//mainVer -> minVer ->builVer
		uint8_t len[4];				//长度,大端格式
		uint8_t chksum[4];			//
		uint8_t encode;				//编码方式，0：无效

		uint8_t boot_en[2];
		uint8_t fwTag[DFU_FWTAG_LEN];

		uint8_t seed[1];			//种子
	};
	uint8_t dats[DFU_HDPKT_LEN];
}DFUHD_PKT;
typedef struct _UPDT_INFO{
    DFUHD2_PKT	hdPkt;
    u8      bin[1];		//bin
	//u8 data[VER_BIN_SIZE];
}UPDT_INFO;
#endif

//--------------------------------------------
#define CTRL_DFU_START 				0x60
#define CTRL_DFU_GOING				0x61
#define CTRL_DFU_FINISH				0x62
#define CTRL_DFU_CANCEL				0x63

#define DFU_HDPKT_VAL32(x)	(u32)((*(x) << 24)+ (*((x) + 1) << 16) + (*((x) + 2) << 8) + *((x) + 3) )
#define DFU_HDPKT_VAL16(x)	(u32)((*(x) << 8)+ *((x) + 1))

//--------ackCode--------------------
#define DFU_ACK_ERR_ID		0x80
#define DFU_ACK_ERR_SUM		0x81
#define DFU_ACK_ERR_FMT		0x82
#define DFU_ACK_ERR_PARAM	0x83
#define DFU_ACK_ERR_VERIFY	0x84
#define DFU_ACK_ERR_PKT		0x85
#define DFU_ACK_ERR_FLSH	0x86
#define DFU_ACK_ERR_STAT	0x87
#define DFU_ACK_UPGRADE_FAID	0x88
//-------------------------------
#define DFU_ACK_OK			0x00
#define DFU_ACK_VERIFY		0x01
#define DFU_ACK_CMPLT		0x02

//---------------------------------
#define DFU_STAT_IDLE		0	
#define DFU_STAT_START		1	
#define DFU_STAT_GOING		2	
#define DFU_STAT_STOP		3	
#define	DFU_STAT_VERFY		4	
#define DFU_STAT_CMPLT		5	

typedef struct _DFU_INFO{
//    UPDT_INFO ver;
	u32 idx;
	u32 offset;
	//u32 chksum;
	//c8 *name;

	//u8 type;
	u8 stat;	//0: none , 1 : start ,
	u8 ackCode;	//ackCode
}DFU_INFO;


//u8 dfu_main(void);
u8 dfu_proc(u8 cmd,u8 *lpDats,u16 len);

#endif

