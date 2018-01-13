/*
 * app_netmsg.c
 *
 *  Created on: Dec 7, 2017
 *      Author: root
 */
#include "app_netmsg.h"
#include "devDbg.h"
#include "app_dev.h"
#include "paramconfig.h"
A_UINT32 swat_bench_quit();


#define UPDATE_SWAT_EXIT	-1
#define UPDATE_LEN_ERR		-2
#define UPDATE_PKT_TIMEOUT	-3
#define UPDATE_PKT_FINISH	0
#define UPDATE_PKT_UNFNSH	1
#define UPDATE_PKT_MSLEEP	2

#define UPDATE_PKT_ERR(c)	((c) < 0)

const MSG_HD gMsgHD_Def = {
	{
		(A_UINT32)NETMSG_ID,	//MSGID
		0,						//MSGNO
		0,						//len
		0,						//code
		NETMSG_VER,				//ver
		0,						//errcode
		{0,0},					//resv[2]
	}
};
static A_INT8 updateNetMsgPktIdx(NET_MSG *lpMsg,A_UINT32 len,A_UINT32 msleep)
{
	if (swat_bench_quit())
		return UPDATE_SWAT_EXIT;

	if(!len && msleep)
	{
		qcom_thread_msleep(msleep);
		return UPDATE_PKT_MSLEEP;
	}
	//pkt proc timeout
	/*
	 if(lpMsg->timer)
	 {
	 }
	 */

	lpMsg->recvPktIdx += len;
	if(len <= lpMsg->datLen )
	{
		lpMsg->lpDats += len;
		lpMsg->datLen -= len;
	}else
	{
		lpMsg->lpDats = 0;
		return UPDATE_LEN_ERR;
	}

	if(lpMsg->recvPktIdx >= (lpMsg->hdPkt.len + NET_MSGHD_SIZE))
	{
		lpMsg->msgProc = NULL;
		lpMsg->recvPktIdx = 0;
		return UPDATE_PKT_FINISH;
	}

	return UPDATE_PKT_UNFNSH;
}

static A_INT32 skip_netMsgPkt(NET_MSG *lpMsg)
{
	A_UINT32 len = (lpMsg->datLen < lpMsg->hdPkt.len) ?
				lpMsg->datLen : lpMsg->hdPkt.len;
	A_INT8 s = updateNetMsgPktIdx(lpMsg,len,0);

	if( s == UPDATE_PKT_UNFNSH)
	{
		if(!lpMsg->datLen)
			lpMsg->msgProc = skip_netMsgPkt;
	}

	return UPDATE_PKT_ERR(s) ? -1 : 0;
}
static A_INT32 skip_netMsgPkt_Rsp(NET_MSG *lpMsg)
{
	HCTRL_PTOKEN *lpToken = getCtrlToken(lpMsg->ctrlHandle);
	A_UINT32 len = (lpMsg->datLen < lpMsg->hdPkt.len) ?
				lpMsg->datLen : lpMsg->hdPkt.len;
	A_INT8 s = updateNetMsgPktIdx(lpMsg,len,0);

	if( s == UPDATE_PKT_UNFNSH)
	{
		if(!lpMsg->datLen)
			lpMsg->msgProc = skip_netMsgPkt_Rsp;
	}else if(lpToken && lpToken->portObj->outStream)
	{
		lpToken->portObj->outStream(NULL,0,lpToken->portObj->funcParam);
	}

	return UPDATE_PKT_ERR(s) ? -1 : 0;
}

A_INT32 syncSendDevPkt(HCTRL_CMDPKT *lpPkt);
static A_INT32 host_netMsg_DsEmul(NET_MSG *lpMsg)
{
	HCTRL_PTOKEN *lpToken = getCtrlToken(lpMsg->ctrlHandle);
	A_INT32 n,i;
	A_UINT8 sendIdx;

	if(!(lpToken && lpMsg->datLen))
	{
		DBG_STR("host_netMsg_DsEmul: ...ERROR\r\n");
		return -1;
	}

	n = (lpMsg->datLen < lpMsg->hdPkt.len) ? lpMsg->datLen : lpMsg->hdPkt.len;
//	DBG_TRACE("host_netMsg_DsEmul: ...datLen(%d),pktLen(%d)\r\n",
	//		lpMsg->datLen,lpMsg->hdPkt.len);

	if( n )
	{
		i = hCtrl_Attach(lpMsg->ctrlHandle,lpMsg->lpDats,n,NULL,0);
		if( i < 0)
		{
			DBG_TRACE("host_netMsg_DsEmul :  hCtrl_Attach Fail(%d)...\r\n",i);
		}

		if(NETMSG_CODE_MODE(lpMsg->hdPkt.code ) == CODE_MODE_DEV)
		{
			if(!lpToken->portObj->outStream)
				goto DSEMUL_EXIT;
			do
			{
				if(HCTRL_RET_OK != searchCtrlPkt(lpToken))
				{
					//DBG_TRACE("host_netMsg_DsEmul : SearchCtrlPkt Fail(%d)...\r\n",lpToken->cmdIdx);
					//DBG_HEX(lpMsg->lpDats,n);
					break;
				}
				i = syncSendDevPkt(&lpToken->cmdPkt);
				if( i < 0)
				{
					DBG_TRACE("host_netMsg_DsEmul : syncSendDevPkt Fail(%d)...\r\n",i);
					break;
				}

				lpToken->sendCnts = (A_UINT16)i;
				sendIdx = 0;
				while(sendIdx < lpToken->sendCnts)
				{
					i = lpToken->portObj->outStream(&lpToken->cmdPkt.buf[sendIdx],
								lpToken->sendCnts - sendIdx, lpToken->portObj->funcParam);
					if(i <= 0)
					{
						DBG_TRACE("host_netMsg_DsEmul : outStream fail(%d)...\r\n",i);
						break;
					}
					sendIdx += i;
				}
			}while(!swat_bench_quit());
		}else
		{
			do
			{
				hCtrlProtoProc(lpMsg->ctrlHandle);
				if (swat_bench_quit())
					break;

				if(lpToken->stat == HCTRL_TOKENSTAT_PARSE)
				{
					//qcom_thread_msleep(100);
				}else
					break;
			}while(1);
		}
	}

DSEMUL_EXIT:

	lpMsg->recvPktIdx += n;
	lpMsg->lpDats += n;
	lpMsg->datLen -= n;
	if(lpMsg->recvPktIdx >= (lpMsg->hdPkt.len + NET_MSGHD_SIZE))
	{
		lpMsg->msgProc = NULL;
		lpMsg->recvPktIdx = 0;
	}else
	{
		lpMsg->msgProc = host_netMsg_DsEmul;
		lpMsg->datLen = 0;
	}

	return 0;
}

A_UINT16 tcp_get_netMsg_Port(void *lp);
static A_INT32 dev_netMsg_Dats(NET_MSG *lpMsg)
{
	HCTRL_PTOKEN *lpToken = getCtrlToken(lpMsg->ctrlHandle);
	A_UINT32 n = 0,sendIdx;
	A_INT32 m;
	A_INT8 i;
	A_UINT8 datRsp[] = {0x01,0x11,0x21,0x31};

	if(NETMSG_CODE_MODE(lpMsg->hdPkt.code ) != CODE_MODE_DEV)
		return -1;

	if( DATA_PORT != tcp_get_netMsg_Port(lpToken->portObj->funcParam))
		return -2;

	while(lpMsg->datLen)
	{
		n = PUT_SPI_TXBUF(lpMsg->lpDats,lpMsg->datLen);
		i = updateNetMsgPktIdx(lpMsg,n,20);
		if( UPDATE_PKT_ERR(i))
			return -1;

		if( i == UPDATE_PKT_FINISH )
		{
			lpToken->sendCnts = (A_UINT16)sizeof(datRsp);

			memcpy(lpToken->cmdPkt.buf,datRsp,lpToken->sendCnts);

			sendIdx = 0;
			while(sendIdx < lpToken->sendCnts)
			{
				m = lpToken->portObj->outStream(&lpToken->cmdPkt.buf[sendIdx],
							lpToken->sendCnts - sendIdx, lpToken->portObj->funcParam);
				if(m <= 0)
				{
					DBG_TRACE("dev_netMsg_Dats : outStream fail(%d)...\r\n",i);
					break;
				}
				sendIdx += m;
			}
			return 0;
		}
	}

	//UPDATE_PKT_UNFNSH
	lpMsg->msgProc = dev_netMsg_Dats;

	return 0;
}

void setClientLogin(NET_MSG  *lpMsg);
static A_INT32 parseSeverRspPkt(NET_MSG *lpMsg)
{
	switch(NETMSG_CODE_TYPE(lpMsg->hdPkt.code))
	{
	case CODE_TYPE_LOGIN:
		if(!lpMsg->hdPkt.errcode )
		{
			setClientLogin(lpMsg);
		}
		break;
	default:
		break;
	}

	return skip_netMsgPkt(lpMsg);
}
//--------------------------------------------
void resetToken(HCTRL_PTOKEN *token);
static A_INT32 parseNetMsgPkt(NET_MSG *lpMsg)
{
	A_INT32 retVal = -1;
	DBG_TRACE("Code Type(%d) \r\n",NETMSG_CODE_MODE(lpMsg->hdPkt.code));
	if(NETMSG_CODE_MODE(lpMsg->hdPkt.code) == CODE_MODE_SVR)
		return parseSeverRspPkt(lpMsg);

	/*if(!(lpMsg->tag & NETMSG_TAG_LOGIN))
	{
		lpMsg->hdPkt.errcode = NETMSG_ERR_NOLOGIN;
		return skip_netMsgPkt_Rsp(lpMsg);
	}*/

	switch(NETMSG_CODE_TYPE(lpMsg->hdPkt.code))
	{
	case CODE_TYPE_DSEMUL:

		resetToken(getCtrlToken(lpMsg->ctrlHandle));

		retVal = host_netMsg_DsEmul(lpMsg);
		break;
	case CODE_TYPE_DATS:
		retVal = dev_netMsg_Dats(lpMsg);
		break;
	default:
		DBG_TRACE("parseNetMsgPkt : Unknown Code Type(%d) \r\n",NETMSG_CODE_TYPE(lpMsg->hdPkt.code));
		lpMsg->hdPkt.errcode = NETMSG_ERR_UNSUPPORT;
		retVal = skip_netMsgPkt_Rsp(lpMsg);
		break;
	}
	return retVal;
}
static A_INT32 matchNetMsgPkt(NET_MSG *lpMsg)
{
	A_UINT8 n;
	const A_UINT8 mID[] = {NETMSG_ID_0,NETMSG_ID_1,NETMSG_ID_2,NETMSG_ID_3};

	//DBG_TRACE("matchNetMsgPkt...datLen(%d)\r\n",lpMsg->datLen);

	while(lpMsg->recvPktIdx < 4 && lpMsg->datLen)
	{
		if(*lpMsg->lpDats == mID[lpMsg->recvPktIdx])
		{
			lpMsg->hdPkt.buf[lpMsg->recvPktIdx] = *lpMsg->lpDats;
			lpMsg->recvPktIdx++;
		}else
		{
			lpMsg->recvPktIdx = 0;
		}

		lpMsg->datLen--;
		lpMsg->lpDats++;
	}

	if( lpMsg->datLen &&
		4 <= lpMsg->recvPktIdx &&
		lpMsg->recvPktIdx < NET_MSGHD_SIZE)
	{
		n = NET_MSGHD_SIZE - lpMsg->recvPktIdx;
		if( lpMsg->datLen < n)
			n = lpMsg->datLen;

		memcpy(&lpMsg->hdPkt.buf[lpMsg->recvPktIdx],lpMsg->lpDats,n);

		lpMsg->hdPkt.errcode = 0;

		lpMsg->recvPktIdx += n;
		lpMsg->lpDats += n;
		lpMsg->datLen -= n;
	}

	if(lpMsg->recvPktIdx >= NET_MSGHD_SIZE)
	{
		lpMsg->msgProc = NULL;
	}else
	{
		lpMsg->msgProc = matchNetMsgPkt;
	}

	return 0;
}

A_INT32 netMsg_proc(NET_MSG *lpMsg)
{
	A_INT32 n;

	//DBG_STR("netMsg_proc1...\r\n");

MSG_START:

	if(lpMsg->msgProc)
		n = lpMsg->msgProc(lpMsg);
	else
	{
		if(lpMsg->recvPktIdx < NET_MSGHD_SIZE)
		{
			lpMsg->stat = NETMSG_STAT_MATCH;
			n = matchNetMsgPkt(lpMsg);
		}else
		{
			lpMsg->stat = NETMSG_STAT_PARSE;
			n = parseNetMsgPkt(lpMsg);
		}
	}

	if(lpMsg->datLen && n >= 0 )
		goto MSG_START;

	return n;
}
void netMsg_reset(NET_MSG* lpMsg)
{
	//DBG_STR("netMsg_reset...\r\n");

	lpMsg->recvPktIdx = 0;
	lpMsg->msgProc = NULL;
	lpMsg->stat = NETMSG_STAT_IDLE;
	lpMsg->datLen = 0;
	lpMsg->tag = 0;
}
void netMsg_init(NET_MSG *lpMsg)
{
	memset(lpMsg,0,sizeof(NET_MSG));
}

