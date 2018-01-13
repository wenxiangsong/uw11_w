#ifndef _DSEMUL_H_
#define _DSEMUL_H_

//#include "typedef.h"
//#include "Tulip.h"
//#include "if.h"
#include "devDbg.h"
/*
#if 1
	#define HCTRL_DBG_TRACE		debug_out
	#define HCTRL_DBG_STR(s)	outStr(s)
#else
	#define HCTRL_DBG_TRACE(fmt,...)
	#define HCTRL_DBG_STR(s)
#endif
*/
#if 1
typedef unsigned char 	u8;
typedef unsigned short  u16;
typedef unsigned long 	u32;
typedef signed long 	s32;
typedef signed short 	s16;
typedef  char 			s8;
typedef const char 		c8;
#endif

//--------------------------------------------------------
#define DEV_RET			s16
//----------------------------------------------------------
#define DEV_RET_OK(f)	((f) >= 0)
#define DEV_RET_FAIL(f)	((f) < 0)

#define DEV_OK			(0)
#define DEV_ERR			(-1)
#define DEV_ERR_NODAT	(-2)

//-----------------------------------------------------------
#define HCTRL_H2L16(n) \
	(u16)(((u16)(n) >> 8) | ((u16)(n) << 8))
#define HCTRL_H2L32(n)	\
	(u32)(((u32)(n) >> 24) | (((u32)(n) & 0xff0000) >> 8) | (((u32)(n) & 0xff00) << 8) | (((u32)(n) & 0xff) << 24))

#define HCTRL_MKVAL32(n) \
	(((u32)(*(n)) << 24) | ((u32)(*((n) + 1)) << 16) | ((u32)(*((n) + 2)) << 8) | (u8)(*((n) + 3)))
#define HCTRL_MKVAL16(n) \
		(((u16)(*(n)) << 8) | (u8)(*((n) + 1)))

#define HCTRL_MKVAL32N(n) \
	((u8)(*(n)) | ((u32)(*((n) + 1)) << 8) | ((u32)(*((n) + 2)) << 16) | (u32)(*((n) + 3) << 24))
#define HCTRL_MKVAL16N(n) \
		((u8)(*(n))  | (u16)((*((n) + 1)) << 8))


#define U32TODATS(lp,d) {	\
	*((u8*)(lp)) = ((d) >> 24);	\
	*((u8*)(lp) + 1) = ((d) >> 16);	\
	*((u8*)(lp) + 2) = ((d) >> 8);	\
	*((u8*)(lp) + 3) = (u8)(d);	\
}

#define HEXTOSTR(lpS,sLen,lpD,dLen)			hexToStr(lpS,sLen,lpD,dLen,0xff)
#define HEXTOSTR_SEP(lpS,sLen,lpD,dLen,sep) hexToStr(lpS,sLen,lpD,dLen,sep)
//---------------------------------
#define DSEMUL_PVER			0x0101	//协议版本号
#define DSEMUL_DVER			0x0101  //设备版本号

//----------------work mode----------------------
#define DSINFO_MODE_NORMAL		0x01//0x00//正常模式
#define DSINFO_MODE_IDLE		0x02	//待机或正常模式
#define DSINFO_MODE_DFU			0x03	//固件升级模式
#define DSINFO_MODE_TEST		0x04	//测试模式
#define DSINFO_MODE_FACTORY		0x05	//工厂模式
#define DSINFO_MODE_SAMPLE		0x06	//数据采样

//-----------MODEL TYPE--------------
#define DS_DEVTYPE_PRINTER		0x01
#define DS_DEVTYPE_HEALTH		0x02
#define DS_DEVTYPE_LED			0x03
#define DS_DEVTYPE_WIFI			0x04

//-----------DEVNO TYPE--------------
#define DS_DATTYPE_STR			0x01
#define DS_DATTYPE_HEX			0x02

//---------------------------------
#define HCTRL_ACK_OK			0x00
#define HCTRL_ERR_UNSUPPORT		0x01	//不支持的指令
#define HCTRL_ERR_SUM			0x02	//校验和不匹配
#define HCTRL_ERR_UNCONNT		0x03	//设备尚未连接
#define HCTRL_ERR_PARAM			0x04	//参数错误
#define HCTRL_ERR_LEN			0x05	//数据长度错误
#define HCTRL_ERR_MATCH			0x06	//设备未连接相应模块
#define HCTRL_ERR_PASSWD		0x07	//密码错误
#define HCTRL_ERR_BUSY			0x08	//设备忙
#define HCTRL_ERR_NODATA		0x09	//没数据
#define HCTRL_ERR_MODE			0x80	//mode error
#define HCTRL_ERR_CMDFAIL		0x81	//执行失败

//---------------------------------------
#define CTRL_CMD_NOUSE			0x00	//
#define CTRL_CMDGET_MODEL		0x01	//获取设备型号
#define CTRL_CMDGET_DEVNO		0x02	//获取设备序列号
#define CTRL_CMDGET_PROTVER		0x03    //获取得实仿真规范版本号
#define CTRL_CMD_CONNT			0x04    //设备连接              
#define CTRL_CMD_UNCONNT		0x05    //设备断开

#define CTRL_CMDSET_PSW			0x06    //设置密码
#define CTRL_CMDGET_USERDAT		0x07    //获取用户数据
#define CTRL_CMDSET_USERDAT		0x08    //设置用户数据
#define CTRL_CMDGET_DEVSTAT		0x09    //获取设备状态
#define CTRL_CMDGET_PWSSTAT		0x0A    //获取加密状态
#define CTRL_CMDSET_ENCRYPT		0x0B    //设置加密
#define CTRL_CMDGET_DEVINFO		0x0C    //获取设备信息
#define CTRL_CMD_RESETCFG		0x0D    //恢复出厂设置
#define CTRL_CMD_CLSBUF			0x0E    //清除缓存
#define CTRL_CMDGET_STATISINFO		0x0F    //获取数据统计
#define CTRL_CMDGET_MAINTAININFO	0x10    //获取设备维修信息      
#define CTRL_CMD_DEVRESTART		0x11    //设置设备重启
#define CTRL_CMD_DEVCHKSLF		0x12    //设备检测
#define CTRL_CMDGET_WORKMODE	0x13    //获取工作模式
#define CTRL_CMDSET_WORKMODE	0x14    //设置工作模式
#define CTRL_CMDGET_VERINFO		0x15    //获取固件信息
#define CTRL_CMD_DFUMODE		0x16    //设置固件升级模式
                                     
#define CTRL_CMDGET_CGFINFOS	0x17    //获取设备配置信息
#define CTRL_CMDSET_CFGINFOS	0x18	//设备配置
#define CTRL_CMD_DATA			0x19	//数据传输
#define CTRL_CMDGET_CFGFMT		0x1A	//获取设备配置格式信息
#define CTRL_CMDGET_SYSPARAM	0x1B	//获取设备系统参数
#define CTRL_CMDSET_SYSPARAM	0x1C	//设置设备系统参数
#define CTRL_CMDSET_OPER		0x1D	//设置设备操作

//#define CTRL_CMDGET_DEVTAG		0x80	//获取设备标识
//#define CTRL_CMD_FASTERASE		0x81	//快速擦除

//--------------------------------------------
#define CTRL_UDF_START 				0x60	//更新开始
#define CTRL_UDF_GOING				0x61	//数据发送
#define CTRL_UDF_FINISH				0x62	//发送完毕
#define CTRL_UDF_CANCEL				0x63	//取消更新

#define CTRL_CMD_TEST				0xf0	//测试命令

#define HCTRL_CMDPKT_HDLEN			4
#define HCTRL_CMDPKT_DATLEN			255
#define HCTRL_CMDPKT_MAXLEN			(HCTRL_CMDPKT_HDLEN + HCTRL_CMDPKT_DATLEN)
#define HCTRL_CMDGET_PKTLEN(pkt)	(((pkt).Len) + HCTRL_CMDPKT_HDLEN)

#define HCTRL_PKT_ID				(0x10)
typedef union _HCTRL_CMD_PACKET{
	struct{
		u8 ID;
		u8 Code;
		u8 Len;
		u8 Sum;
		u8 dats[HCTRL_CMDPKT_DATLEN];
	};
	u8 buf[HCTRL_CMDPKT_MAXLEN];
}HCTRL_CMDPKT;

//-------------------------------
//#define DL210_DEVID			0x01
//#define DSEMUL_CFGID		0x05
#define HCTRL_GETCFG_DATLEN(lpPkt)		(((lpPkt)->cfgLen_H << 8) + (lpPkt)->cfgLen_L)
#define HCTRL_GETCFG_PKTLEN(lpPkt)	(HCTRL_GETCFG_DATLEN(lpPkt) + sizeof(DEVCFG_BASE))
#define HCTRL_GETCFG_DATS(lpPkt)	(u8*)((u32)(lpPkt) + sizeof(DEVCFG_BASE))
typedef struct _DEVCFG_BASE_DESC{
	u8 devID;
	u8 cfgID;
	u8 cfgLen_H;
	u8 cfgLen_L;
	u8 cfgVer;
	u8 crc8;
	u8 resv[2];
}DEVCFG_BASE;


#endif
