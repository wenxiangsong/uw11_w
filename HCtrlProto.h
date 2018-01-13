#ifndef _HOST_CTRL_
#define _HOST_CTRL_

//#include "dsEmulCmd.h"
#include "wiDSEmul.h"
#include "qcom/qcom_time.h"
#ifndef NULL
#define NULL (void*)
#endif

#define DSEMUL_DBG_STR(s)	DBG_STR(s)
#define HCTRL_PBUF_REMAIN	(2 * 1024)	//接收缓冲满的预留空间
//------------------------------------------------
#define HCTRL_HL2BYTE(n) {((n) >> 8),((n) & 0xff)}

//extern void SystemDelay1mS(u32 DelayNumber);
#define HCTRL_DELAY_MS(t)		us_delay( (t) * 1000)//BIOS_DELAY_TIME(t)//SystemDelay1mS(t)

//---------------------------------
//#define HCTRL_PORT_NUMS 		1
#define HCTRL_TOKEN_NUMS		3

#define HCTRL_PORTBUF_MINLEN	1024

#define HCTRL_RS_MODE			0
#define HCTRL_TX_MODE			1
#define HCTRL_PHANDLE			u8
#define HCTRL_PHANDLE_INVALID	0xff

//----------type--------------------
#define HCTRL_PTYPE_NONE		0
#define HCTRL_PTYPE_USED		0x01

//----------------------------------
#define HCTRL_TXBUF_EMPTY(p)	(!((p)->txInIdx))// == (p)->txOutIdx)
#define HCTRL_TXBUF_UNEMPTY(p)	((p)->txInIdx )//!= (p)->txOutIdx)
#define HCTRL_RSBUF_EMPTY(p)	(!((p)->rsInIdx)// == (p)->rsOutIdx)
#define HCTRL_RSBUF_UNEMPTY(p)	((p)->rsInIdx)// != (p)->rsOutIdx)

typedef s32 (*HCTRL_OUTSTREAM)(u8* buf,u16 len,void *param);
typedef struct _HCTRL_PORT{
	HCTRL_OUTSTREAM	outStream;
	void *funcParam;
	
	u8 *rsBuf;
	u8 *txBuf;

	//u16 rsLen;
	//u16 txLen;
	//u16 blkDatSize;	//每次发送块最大长度(字节)
	
	u16	rsInIdx;
	u16 rsOutIdx;
	
	u16 txInIdx;
	u16 txOutIdx;
	
	u8 type;
}HCTRL_PORT;

#define HCTRL_RET			s32
#define HCTRL_RET_IDLE		(1)		//等待数据
#define HCTRL_RET_OK		(0)

#define	HCTRL_CMPLT_TOKEN(token) 

struct _HCTRL_PTOKEN;

typedef HCTRL_RET (*HCTRL_FUNC)( struct _HCTRL_PTOKEN* lpToken);

//------------tag---------------
#define HCTRL_TOKEN_INUSE	0x01
//------------stat--------------
#define HCTRL_TOKENSTAT_IDLE 	0x00 //
#define HCTRL_TOKENSTAT_START	0x01
#define HCTRL_TOKENSTAT_WAITING	0x02
#define HCTRL_TOKENSTAT_PARSE	0x03
#define HCTRL_TOKENSTAT_CMPLT	0x04

typedef struct _HCTRL_PTOKEN{
	HCTRL_PORT	*portObj;
	HCTRL_FUNC	tokenProc;
	
	HCTRL_CMDPKT cmdPkt;
	u16	cmdIdx;
	u16 sendCnts;
	
	u8	tag;
	u8 stat;
}HCTRL_PTOKEN;

HCTRL_RET HCTRL_CMD_Ack(HCTRL_PTOKEN *token,u8 errNo);
void mkSendPkt(HCTRL_PTOKEN *token);
HCTRL_RET sendPortDat(HCTRL_PTOKEN *token);

//HCTRL_PHANDLE regCtrlPort(u8 *rsBuf,u16 rsLen,u8 *txBuf,u16 txLen,HCTRL_OUTSTREAM outFunc,u16 blkSize);
//HCTRL_PHANDLE regCtrlPort(u8 *rsBuf,u8 *txBuf,HCTRL_OUTSTREAM outFunc,void *param);
HCTRL_PHANDLE regCtrlPort(HCTRL_PORT* lpPort);
u8 unRegCtrlPort(u8 handle);
void initCtrlPort(HCTRL_PORT* lpPort,u8 *rsBuf,u8 *txBuf,HCTRL_OUTSTREAM outFunc,void *param);
HCTRL_RET searchCtrlPkt(HCTRL_PTOKEN *token);
HCTRL_RET hCtrlProtoProc(u8 handle);
HCTRL_PHANDLE hCtrl_Attach(HCTRL_PHANDLE handle,u8 *rsBuf,u16 rsDats,u8* txBuf,u16 txBufLen);

// 将设备端口输入的数据保存到输入缓冲中(rsBuf)
s32 HCtrlPortDataIn(HCTRL_PHANDLE pHandle,u8 *d,u16 len);
// 将输出缓冲数据由设备端口发出(txBuf)
s32 HCtrlPortDataOut(HCTRL_PHANDLE pHandle,u8 *d,u16 len);
// 将数据保存到输出缓冲
s32 HCtrlPutDat2TxBuf(u8 *d,u16 len);
HCTRL_PTOKEN *getCtrlToken(u8 handle);
u8 getCtrlTokenStat(u8 handle);
void initCtrlProto(void);
u16 mkHCtrlPkt(HCTRL_CMDPKT *lpPkt,u8 cmd,u8 *lpD,u8 len);

//----------------emulStat-------------------
#define DSEMUL_STAT_CONNECTED	0x01 //命令仿真处于连接状态

typedef struct _DSEMUL_INFO{
	
	u8 mode;	//
	
	u8 emulStat;
	
	u8	errNo;
	
}DSEMUL_INFO;


//----------------设备静态属性信息-------------------
//#define DEVINFO_FMTTYPE_WIFI	(0x0001)
//#define PARAMPROP_FMTTYPE_WIFI	(0x0001)
//#define DEVSTAT_FMTTYPE_WIFI	(0x0001)

//设备属性类型 ：0x01 ~ 0x2f
//系统状态类型 ：0x30 ~ 0x3f
//系统参数类型 ：0x81 ~ 0x9f
#define DEVINFO_TYPE_NONE		0
#define	DEVINFO_TYPE_GEN		0x01	//通用信息
#define DEVINFO_TYPE_USER		0x02	//user info
#define DEVINFO_TYPE_CNTS		2
//大端格式
typedef struct _DSEMUL_DEVINFO_ITEM{
	u8 size;
	u8 itemType;	//
	u8 info_dats[1];
}DSEMUL_DEVINFO_ITEM;

#define DEVINFO_TYPE_WIFI	(0x0080)
typedef struct _DSEMUL_DEVICE_INFO{
	u8 infoType[2];
	u8 itemCnts;
	u8 revs;
	DSEMUL_DEVINFO_ITEM infoItem[1];
}DSEMUL_DEV_INFO;

#define DEVINFO_GEN_SEP	(';')
typedef struct _DSEMUL_DEVINFO_GEN{
	u8 size;
	u8 itemType;		//	DSEMUL_SYSINFO_GEN
	u8 sep;
		//设备型号;序列号;通讯版本号;固件版本号;主板编号;备注;
	u8 info_desc[1];	//240
}DSEMUL_DEVINFO_GEN;

typedef struct _DSEMUL_DEVINFO_USER{
	u8 size;
	u8 itemType;		//	DEVINFO_TYPE_USER
	u8 userName[1];		// BYTE[0] = len,BYTE[1 ~ len] = name
}DSEMUL_DEVINFO_USER;


//-------设备运行时的系统参数属性----------------
//#define BMP_DEVPROP_NONE		0xff
//#define BMP_DEVPROP_PRN			0x81	//print out param
//#define SYSP_TYPE_CNTS			1

//-------系统状态--------------------------
#define DEVSTAT_TYPE_GEN	0x30	//系统状态
#define DEVSTAT_TYPE_FLOW	0x31	//数据接收、发送状态
#define DEVSTAT_TYPE_CNTS	2

//-------GEN_STAT------------
#define GEN_SYSSTAT_IDLE	0	//空闲，等待指令
#define GEN_SYSSTAT_WORKING	1	//
#define GEN_SYSSTAT_READY	2	//就绪
#define GEN_SYSSTAT_BUSY	3	//数据将满
#define GEN_SYSSTAT_PAUSE	4	//暂停

//#define GEN_SYSSTAT_ERR		0xff//异常
#define UDP_PORT_IDX	0
#define CTRL_PORT_IDX	1
#define DATA_PORT_IDX	2

typedef struct _DSEMUL_GEN_STAT{
	u8 size;
	u8 itemType;			//	DEVSTAT_TYPE_GEN
	u8 sysMode;				// 系统运行状态
	u8 wifiMode;
	u8 devStat;
	u8 errCode[2];			//错误号
	u8 portStat[3];			// 0:udp , 1: ctrl_port , 2: data_port
}DSEMUL_UW_GENSTAT;

typedef struct _DSEMUL_FLW_STAT{
	u8 size;
	u8 itemType;			//	DEVSTAT_TYPE_FLOW
	u8 flwStat;
	u8 recvSpace[4];		//接收缓冲的剩余空间
	u8 recvRate[2];			//接收缓冲的数据比例(1/1000)
}DSEMUL_UW_FLWSTAT;

#if 0
extern DSEMUL_UW_FLWSTAT gUWFlwStat;
extern DSEMUL_UW_GENSTAT gUWSysStat;
#define GEN_SET_SYSMODE(s)	do{gUWSysStat.sysMode = (s);}while(0)
#define GEN_SET_WIFIMODE(s)	do{gUWSysStat.wifiMode = (s);}while(0)
#define GEN_SET_ERRCODE(e)	do{gUWSysStat.errCode[0] = (u8)((e) >> 8) ;\
								gUWSysStat.errCode[0] = (u8)(e) ;}while(0)
*/
//-------------------------
#define UDP_PORT_IDX	0
#define CTRL_PORT_IDX	1
#define DATA_PORT_IDX	2

#define PSTAT_OPENED	0x01	//opened
#define GEN_SET_PSTAT(s,i)	do{gUWSysStat.portStat[i] = (s);}while(0)
#define GET_SET_UDPSTAT(s)	GEN_SET_PSTAT(s,UDP_PORT_IDX)
#define GET_SET_CTRLSTAT(s)	GEN_SET_PSTAT(s,CTRL_PORT_IDX)
#define GET_SET_DATASTAT(s)	GEN_SET_PSTAT(s,DATA_PORT_IDX)

#define GEN_CLS_PSTAT(s,i)	do{gUWSysStat.portStat[i] &= ~(s);}while(0)
#define GET_CLS_UDPSTAT(s)	GEN_SET_PSTAT(s,UDP_PORT_IDX)
#define GET_CLS_CTRLSTAT(s)	GEN_SET_PSTAT(s,CTRL_PORT_IDX)
#define GET_CLS_DATASTAT(s)	GEN_SET_PSTAT(s,DATA_PORT_IDX)

#define GEN_SET_RCVSPACE(s)	do{\
								gUWFlwStat.recvSpace[0] = (u8)((s) >> 24) ;\
								gUWFlwStat.recvSpace[1] = (u8)((s) >> 16) ;\
								gUWFlwStat.recvSpace[2] = (u8)((s) >> 8) ;\
								gUWFlwStat.recvSpace[3] = (u8)(e) ;\
								}while(0)
#define GEN_SET_RCVRATE(r)	do{gUWFlwStat.errCode[0] = (u8)((r) >> 8) ;\
								gUWFlwStat.errCode[1] = (u8)(r) ;}while(0)
#endif
/*
typedef struct _DSEMUL_FAULT_STAT{
	u8 size;
	u8 itemType;			//	DEVSTAT_TYPE_FAULT
	u8 faultNo;				//

}DSEMUL_FAULT_STAT;

typedef struct _DSEMUL_PORT_STAT{
	u8 size;
	u8 itemType;		//	DEVSTAT_TYPE_PORT

	u8 portStat;		//
	u8 recvRate[2];		//接收缓冲的数据比例(1/1000)
	u8 recvSpace[4];	//接收缓冲的剩余空间
	u8 sendDats[4];

}DSEMUL_PORT_STAT;
*/
/*
//--------------------------------------------------------
#define DEV_RET			s16
//----------------------------------------------------------
#define DEV_RET_OK(f)	((f) >= 0)
#define DEV_RET_FAIL(f)	((f) < 0)

#define DEV_OK			(0)
#define DEV_ERR			(-1)
#define DEV_ERR_NODAT	(-2)
*/
//-----------------------------------------------------------
void qcom_sys_reset(void);

#define DEV_CTRL_INIT()					uw_dev_init()
#define DEV_GET_MODEL(lp,len,idx)		uw_getModel(lp,len,idx)
#define DEV_SET_PSW(lp,len)				DEV_ERR
#define DEV_GET_USERDAT(idx,lp,len)		DEV_ERR
#define DEV_SET_USERDAT(idx,lp,len)		DEV_ERR
#define DEV_GET_DEVSTAT(idx,lp,len)		uw_getDevStat(idx,lp,len)
#define DEV_GET_DEVINFO(idx,lp,len)		uw_getDevInfo(idx,lp,len)
#define DEV_CMD_CLSBUF()				DEV_ERR
#define DEV_GET_STATISINFO(lp,len)		DEV_ERR
#define DEV_CMD_CHKSLF(lp,len)			DEV_ERR
#define DEV_GET_WORKMODE(lp,len)		DEV_ERR
#define DEV_SET_WORKMODE(lp,len)		DEV_ERR
#define DEV_GET_VERINFO(lp,len)			uw_getVerInfo(lp,len)
#define DEV_GET_CFGINFO(idx,lp,len)		uw_getCfgInfo(idx,lp,len)
#define DEV_GET_CFGFMT(idx,lp,len)		DEV_ERR
#define DEV_SET_CFGINFO(idx,lp,len)		uw_setCfgInfo(idx,lp,len)
#define DEV_GET_SYSPARAM(idx,lp,len)	DEV_ERR
#define DEV_SET_SYSPARAM(lp,len)		DEV_ERR
#define DEV_RESET_CFGINFO(lp,len) 		DEV_ERR
#define DEV_CMD_UDF(lp,len)				DEV_ERR
#define DEV_SOFT_RESTART()				qcom_sys_reset()
#define DEV_SET_DEVOPER(idx,lp,len)		DEV_ERR
#define DEV_GET_DEVTAG(idx,lp,len)		DEV_ERR
#define DEV_CMD_FASTERASE(idx,lp,len)	DEV_ERR
#define DEV_CONNT(lp,len)				uw_dev_connt(lp,len)

#endif
