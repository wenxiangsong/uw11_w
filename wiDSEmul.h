#ifndef _WIDSEMUL_H_
#define _WIDSEMUL_H_

#include "dsEmulCmd.h"

#define FW_VER		0x0009
#define DEV_MODEL	"UW11_W"
#define DEV_SERIAL	"******"
#define DEV_MBNO	"STM32F4-QCA4"

#define SN_ID_Model		0x00
#define SN_ID_Serial	0x01

//------------DEVCFG-devID------------------
#define GENCFG_DEVID		0xFE

#define UWCFG_SYS_CFGID		0x01
#define UWCFG_SYS_CFGVER	0x01

//----------------------------

#define DSEMUL_CFG_PKTLEN		sizeof(DSEMUL_UWCFG)
#define DSEMUL_CFG_DATLEN 		(DSEMUL_CFG_PKTLEN - sizeof(DEVCFG_BASE))
//#define DSEMUL_PSW_LEN			16

typedef struct _DSCFG_INFO{
	DEVCFG_BASE base;
	u8 dat[1];
	//u8 userPsw[DSEMUL_PSW_LEN];
}DSEMUL_UWCFG;

#define BT_MBNO_LEN		16
#define DSVER_MODEL_LEN	32
#define DFU_FWTAG_LEN	32
typedef struct _DSVER_INFO{
	u32 fwVer;
	u16 btVer;

	u16	dsEmulPVer;
	u16 dsEmulDVer;

	u8  fwTag[DFU_FWTAG_LEN];	//
	u8	fwDateStamp[16];	//fw
	u8	fwTimeStamp[8];		//fw
	u8	btDateStamp[16];	//boot
	u8	btTimeStamp[8];		//boot

	u8	boardNo[BT_MBNO_LEN];	//
	u8	model[DSVER_MODEL_LEN];
	u8	serialNo[DSVER_MODEL_LEN];	//
}UW_VERINFO;

DEV_RET uw_cmdDFU(u8 *lp,u16 len);
//DEV_RET uw_clsBuf(void);
DEV_RET uw_getModel(u8 *lpDats,u16 len,u8 idIdx);
//DEV_RET uw_setPsw(u8 *lpDats,u16 len);
//DEV_RET uw_getUserData(u16 idx,u8 *lpDats,u16 len);
//DEV_RET uw_setUserData(u16 idx,u8 *lpDats,u16 len);
DEV_RET uw_getDevStat(u8 idx,u8 *lpDats,u16 len);
DEV_RET uw_getDevInfo(u8 idx,u8 *lpDats,u16 len);
//DEV_RET uw_getStatisInfo(u8 *lpDats,u16 len);
//DEV_RET uw_chkSlf(u8 *lpDats,u16 len);
DEV_RET uw_getWorkMode(u8 *lpDats,u16 len);
DEV_RET uw_setWorkMode(u8 *lpDats,u16 len);
DEV_RET uw_getVerInfo(u8 *lpDats,u16 len);
DEV_RET uw_getCfgInfo(u16 idx,u8 *lpDats,u16 len);
//DEV_RET uw_getCfgFmt(u16 idx,u8 *lpDats,u16 len);
DEV_RET uw_setCfgInfo(u16 idx,u8 *lpDats,u16 len);
DEV_RET uw_setSysParam(u8 *lpDats,u16 len);
DEV_RET uw_getSysParam(u16 idx,u8 *lpDats,u16 len);
DEV_RET uw_resetCfgInfo(u8 *lpDats,u16 len);
//DEV_RET uw_devOperate(u8 idx,u8 *lpDats,u16 len);
//DEV_RET uw_getDevTag(u8 idx,u8 *lpDats,u16 len);
//DEV_RET uw_cmdFastErase(u8 idx,u8 *lpDats,u16 len);
DEV_RET uw_dev_connt(u8 *lp,u16 len);
void uw_dev_init(void);

#endif 
