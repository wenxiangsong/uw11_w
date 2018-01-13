//#include "wiDSEmul.h"
#include "paramconfig.h"
#include "HCtrlProto.h"

extern struct sys_config g_sys_config;
extern A_UINT8 currentDeviceId;
//DSEMUL_UW_GENSTAT gUWSysStat;
//DSEMUL_UW_FLWSTAT gUWFlwStat;

/*多项式为：X8+X5+X4+1*/
static unsigned char encode_crc8(unsigned char *pData, int nLength)
{
    unsigned char crc;
    unsigned char i;
    crc = 0;
    while(nLength--)
    {
       crc ^= *pData++;
       for(i = 0;i < 8;i++)
       {
           if(crc & 0x80)
           {
               crc = (crc << 1) ^ 0x31;
           }else crc <<= 1;
       }
    }
   return crc;
}
const char HEX_CHAR[] = {"0123456789ABCDEF"};
unsigned short hexToStr(char *lp,unsigned short bufLen,
						const unsigned char *hex,unsigned short datLen,
						unsigned char sep)
{
	unsigned short i,j;

	for(i = 0,j = 0; j < bufLen && i < datLen ; i++)
	{
		lp[j++] = HEX_CHAR[(hex[i] >> 4)];
		if(j < bufLen)
			lp[j++] = HEX_CHAR[(hex[i] & 0x0f)];
		if(sep != 0xff && j < bufLen)
			lp[j++] = sep;
	}

	if( j < bufLen)
		lp[j++] = 0;
	else if(j)
		lp[j - 1] = 0;

	return j;
}

u8 mkDSCfgPkt(DEVCFG_BASE *lpCfg,u8 *lpD,u16 len,u8 cfgID,u8 ver)
{
	lpCfg->devID = GENCFG_DEVID;
	lpCfg->cfgID = cfgID;
	lpCfg->cfgVer = ver;
	lpCfg->cfgLen_H = (len >> 8);
	lpCfg->cfgLen_L = (u8)len;
	lpCfg->crc8 = encode_crc8(lpD,len);
	lpCfg->resv[0] = lpCfg->resv[1] = 0xff;

	return 1;
}
u8 chkDSCfgPkt(const DEVCFG_BASE *lpCfg)
{
	if(lpCfg->devID != GENCFG_DEVID)
		return 1;

	if(lpCfg->cfgID != UWCFG_SYS_CFGID)
		return 2;

	if(lpCfg->cfgVer != UWCFG_SYS_CFGVER)
		return 3;

	if(lpCfg->crc8 != encode_crc8(HCTRL_GETCFG_DATS(lpCfg),HCTRL_GETCFG_DATLEN(lpCfg)))
		return 4;

	return 0;
}

DEV_RET uw_cmdDFU(u8 *lp,u16 len)
{
	return DEV_OK;
}

DEV_RET uw_getModel(u8 *lpDats,u16 len,u8 idIdx)
{
	if(idIdx == SN_ID_Model)
	{
		lpDats[0] = DS_DEVTYPE_WIFI;
		strncpy((char*)&lpDats[1],(const char*)DEV_MODEL,len - 1);
	}else
	{
		lpDats[0] = DS_DATTYPE_STR;
		strncpy((char*)&lpDats[1],(const char*)DEV_SERIAL,len - 1);
	}

	lpDats[len - 1] = 0;
	return strlen((const char*)lpDats) + 2;
}
void getSysStat(DSEMUL_UW_GENSTAT *sysStat);
static u16 uw_getDevStat_Gen(u8 *lpD,u16 len)
{
	DSEMUL_UW_GENSTAT sysStat;

	getSysStat(&sysStat);
	if(len < sysStat.size )
		return 0;

	memcpy(lpD,(u8*)&sysStat,sysStat.size);

	return sysStat.size;
}
void getFlwStat(DSEMUL_UW_FLWSTAT *fStat);
static u16 uw_getDevStat_Flow(u8 *lpD,u16 len)
{
	DSEMUL_UW_FLWSTAT flwStat;

	getFlwStat(&flwStat);
	if(len < flwStat.size)
		return 0;

	memcpy(lpD,(u8*)&flwStat,flwStat.size);

	return flwStat.size;
}

DEV_RET uw_getDevStat(u8 idx,u8 *lpDats,u16 len)
{
	DSEMUL_DEV_INFO *lpDevInfo = (DSEMUL_DEV_INFO*)lpDats;
	u16 i,n;

	if(idx == 0xff)
	{
		if(len < 5)
			return DEV_ERR;

		lpDats[0] = (DEVINFO_TYPE_WIFI >> 8);
		lpDats[1] = DEVINFO_TYPE_WIFI;
		lpDats[2] = DEVSTAT_TYPE_CNTS;
		lpDats[3] = DEVSTAT_TYPE_GEN;
		lpDats[4] = DEVSTAT_TYPE_FLOW;
		return 5;
	}

	i = 0;
	if(idx == DEVINFO_TYPE_NONE)
	{
		if(len < 4)
			return DEV_ERR;

		lpDevInfo->infoType[0]= (DEVINFO_TYPE_WIFI >> 8);
		lpDevInfo->infoType[1] = DEVINFO_TYPE_WIFI;
		lpDevInfo->itemCnts = 0;
		lpDevInfo->revs = 0;
		len -= 4;
		i = 4;
	}

	n = 0;
	switch(idx)
	{
	case DEVINFO_TYPE_NONE:
	case DEVSTAT_TYPE_GEN:
		n = uw_getDevStat_Gen(lpDats + i,len - i);
		if(!n)
			break;

		i += n;
		if(idx != DEVINFO_TYPE_NONE)
			break;
			lpDevInfo->itemCnts++;
	case DEVSTAT_TYPE_FLOW:
		n = uw_getDevStat_Flow(lpDats + i,len - i);
		if(!n)
			break;
		i += n;
		if(idx != DEVINFO_TYPE_NONE)
			break;
		lpDevInfo->itemCnts++;
		break;
	default:
		return DEV_ERR;
		break;
	}

	return i ? i : DEV_ERR;
}
//设备型号;序列号;通讯协议版本号;固件版本号;主板编号;macAddr;ssid;备注;
static u16 uw_getDevInfo_Gen(u8 *lpD,u16 len,u8 cnts)
{
	//DSEMUL_DEVINFO_GEN *lpGen = (DSEMUL_DEVINFO_GEN*)lpD;
	u32 i;
	s8 *lp;
	u16 idx,n,m;
	s8 buf[32];
	u8 dat[6];

	n = 0;
	idx = 3;
	lp = NULL;
	while(idx < len && n < cnts)
	{
		switch(n)
		{
		case 0:	//设备型号
			lp = (s8*)DEV_MODEL;
			break;
		case 1:	//序列号
			lp = (s8*)DEV_SERIAL;
			break;
		case 2://通讯协议版本号
			dat[0] = (u8)(DSEMUL_PVER >> 8);
			dat[1] = (u8)DSEMUL_PVER;
			dat[2] = (u8)(DSEMUL_DVER >> 8);
			dat[3] = (u8)DSEMUL_DVER;
			HEXTOSTR_SEP(buf,sizeof(buf),dat,4,'.');
			lp = buf;
			break;
		case 3://固件版本号
			i = FW_VER;
			i = HCTRL_H2L32(i);
			lp = buf;
			HEXTOSTR_SEP(buf,sizeof(buf),(const u8*)&i,4,'.');
			break;
		case 4://主板编号
			lp = (s8*)DEV_MBNO;
			break;
		case 5://mac addr
			qcom_mac_get(currentDeviceId, dat);
			lp = buf;
			HEXTOSTR(buf,sizeof(buf),dat,6);
			break;
		case 6://ssid
			lp = g_sys_config.ap_ssid;
			break;
		default:
			break;
		}

		if(lp)
		{
			//m = strnlen((s8*)lp,len - idx);
			m = strlen((s8*)lp);
			if((idx + m) < len)
			{
				strcpy((s8*)&lpD[idx],lp);
				idx += m;
			}else
				break;
		}
		n++;
		if( (idx + 1) < len && n < cnts)
		{
			lpD[idx++] = DEVINFO_GEN_SEP;
		}
	}

	if(idx > 3)
	{
		lpD[0] = idx;
		lpD[1] = DEVINFO_TYPE_GEN;
		lpD[2] = DEVINFO_GEN_SEP;
	}else
		idx = 0;

	return idx;
}
static u16 uw_getDevInfo_User(u8 *lpD,u16 len)
{
	u16 n = strlen(g_sys_config.user_logName);

	if(!n || len < (n + 3))
		return 0;

	lpD[0] = n + 3;
	lpD[1] = DEVINFO_TYPE_USER;
	memcpy(&lpD[2],g_sys_config.user_logName,n + 1);

	return n + 3;
}
DEV_RET uw_getDevInfo(u8 idx,u8 *lpDats,u16 len)
{
	DSEMUL_DEV_INFO *lpDevInfo = (DSEMUL_DEV_INFO*)lpDats;
	u16 i,n;

	if(idx == 0xff)
	{
		if(len < 5)
			return DEV_ERR;

		lpDats[0] = (DEVINFO_TYPE_WIFI >> 8);
		lpDats[1] = DEVINFO_TYPE_WIFI;
		lpDats[2] = DEVINFO_TYPE_CNTS;
		lpDats[3] = DEVINFO_TYPE_GEN;
		lpDats[4] = DEVINFO_TYPE_USER;
		return 5;
	}

	i = 0;
	if(idx == DEVINFO_TYPE_NONE)
	{
		if(len < 4)
			return DEV_ERR;

		lpDevInfo->infoType[0]= (DEVINFO_TYPE_WIFI >> 8);
		lpDevInfo->infoType[1] = DEVINFO_TYPE_WIFI;
		lpDevInfo->itemCnts = 0;
		lpDevInfo->revs = 0;
		len -= 4;
		i = 4;
	}

	n = 0;
	switch(idx)
	{
	case DEVINFO_TYPE_NONE:
	case DEVINFO_TYPE_GEN:
		n = uw_getDevInfo_Gen(lpDats + i,len - i,7);
		if(!n)
			break;

		i += n;
		if(idx != DEVINFO_TYPE_NONE)
			break;

		lpDevInfo->itemCnts++;
	case DEVINFO_TYPE_USER:
		n = uw_getDevInfo_User(lpDats + i,len - i);
		if(!n)
			break;
		i += n;
		if(idx != DEVINFO_TYPE_NONE)
			break;

		lpDevInfo->itemCnts++;
		break;
	default:
		return DEV_ERR;
		break;
	}

	return i ? i : DEV_ERR;
}
DEV_RET uw_getWorkMode(u8 *lpDats,u16 len)
{
	return DEV_OK;
}
DEV_RET uw_setWorkMode(u8 *lpDats,u16 len)
{
	return DEV_OK;
}
DEV_RET uw_getVerInfo(u8 *lpDats,u16 len)
{
	UW_VERINFO *lpVerInfo = (UW_VERINFO*)lpDats;
	//u16 m;
	u8 n = sizeof(UW_VERINFO);

	if(len < n)
		return DEV_ERR;

	memset(lpDats,0,n);

	//m = HCTRL_H2L16(FW_VER);
	lpVerInfo->fwVer = FW_VER;//HCTRL_H2L16(m);
	lpVerInfo->fwVer = HCTRL_H2L32(lpVerInfo->fwVer);

	lpVerInfo->dsEmulPVer = HCTRL_H2L16(DSEMUL_PVER);
	lpVerInfo->dsEmulDVer = HCTRL_H2L16(DSEMUL_DVER);

	qcom_firmware_version_get((char*)lpVerInfo->fwDateStamp,
			(char*)lpVerInfo->fwTimeStamp, (char*)lpVerInfo->btDateStamp, (char*)lpVerInfo->btTimeStamp);

	strncpy((char*)lpVerInfo->boardNo,(const char*)DEV_MBNO,BT_MBNO_LEN);

	strncpy((char*)lpVerInfo->model,(const char*)DEV_MODEL,sizeof(lpVerInfo->model));
	strncpy((char*)lpVerInfo->serialNo,(const char*)DEV_SERIAL,sizeof(lpVerInfo->serialNo));

	return n;
}
DEV_RET uw_getCfgInfo(u16 idx,u8 *lpDats,u16 len)
{
	if(idx && len < sizeof(UW_SYSCFG))
		return DEV_ERR;

	return (DEV_RET)ReadDevConfig((UW_SYSCFG*)lpDats);
}

DEV_RET uw_setCfgInfo(u16 idx,u8 *lpDats,u16 len)
{
	if(idx == 0xffff)
		return DEV_OK;

	if(idx && len < sizeof(UW_SYSCFG))
		return DEV_ERR;

	DBG_STR("uw_setCfgInfo...\r\n");

	DBG_HEX(lpDats,len);

	return WriteDevConfig((UW_SYSCFG*)lpDats) ? DEV_ERR : DEV_OK;
}
DEV_RET uw_setSysParam(u8 *lpDats,u16 len)
{
	return DEV_OK;
}
DEV_RET uw_getSysParam(u16 idx,u8 *lpDats,u16 len)
{
	return DEV_OK;
}
DEV_RET uw_resetCfgInfo(u8 *lpDats,u16 len)
{
	return DEV_OK;
}
DEV_RET uw_dev_connt(u8 *lp,u16 len)
{
	return DEV_OK;
}
void uw_dev_init(void)
{
//	memset((void*)&gUwSysStat,0,sizeof(gUwSysStat));
//	memset((void*)&gUwFlwStat,0,sizeof(gUwFlwStat));
}
