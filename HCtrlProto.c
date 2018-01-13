#include "HCtrlProto.h"
#include "dfu.h"

//--------------------------------------------
//static HCTRL_PORT gCtrlPort[HCTRL_PORT_NUMS];
static HCTRL_PTOKEN gCtrlToken[HCTRL_TOKEN_NUMS];

//--------------------------------------------
static DSEMUL_INFO gDSInfo;//[HCTRL_TOKEN_NUMS];
//static DSEMUL_SMPL	gDSSmpl;

static HCTRL_RET getCtrlPkt(HCTRL_PTOKEN *token);

static void clsPortBuf(HCTRL_PORT *p)
{
	p->rsInIdx = p->rsOutIdx = 0;
	//p->txInIdx = p->txOutIdx = 0;

	p->txOutIdx = 0;
}

void resetToken(HCTRL_PTOKEN *token)
{
	//token->portObj = NULL;
	token->tokenProc = NULL;
	token->cmdIdx = 0;
	token->sendCnts = 0;
	token->stat = HCTRL_TOKENSTAT_IDLE;

}
u16 mkHCtrlPkt(HCTRL_CMDPKT *lpPkt,u8 cmd,u8 *lpD,u8 len)
{
	u8 i;

	lpPkt->ID = HCTRL_PKT_ID;
	lpPkt->Code = cmd;
	lpPkt->Len = len;
	lpPkt->Sum = 0;

	for( i = 0; i < len; i++)
	{
		lpPkt->dats[i] = *lpD;
		lpPkt->Sum += *lpD++;
	}

	return len + HCTRL_CMDPKT_HDLEN;
}
void mkSendPkt(HCTRL_PTOKEN *token)
{
	u16 i = 0;

	token->cmdPkt.Sum = 0;
	while (i < token->cmdPkt.Len)
		token->cmdPkt.Sum += token->cmdPkt.dats[i++];

	token->cmdIdx = 0;
	token->sendCnts = HCTRL_CMDGET_PKTLEN(token->cmdPkt);
}

static HCTRL_RET cmpltToken(HCTRL_PTOKEN *token)
{
	token->tokenProc = getCtrlPkt;
	token->cmdIdx = 0;
	token->sendCnts = 0;

	if(token->portObj->rsOutIdx >= token->portObj->rsInIdx )
	{
		token->stat = HCTRL_TOKENSTAT_CMPLT;
	}

	return HCTRL_RET_OK;
}

static s16 getPortDat(HCTRL_PORT *portObj)//u8 handle)
{
	u8 d;

	//if(gCtrlToken.portObj->rsInIdx == gCtrlToken.portObj->rsOutIdx)
	if (portObj->rsInIdx <= portObj->rsOutIdx)
		return -1;

	d = portObj->rsBuf[portObj->rsOutIdx++];
	//if(gCtrlToken.portObj->rsOutIdx >= gCtrlToken.portObj->rsLen)
	//gCtrlToken.portObj->rsOutIdx = 0;
	return d;
}
static u16 putDat2TxBuf(HCTRL_PORT *port, u8 *d, u16 len)
{
	u16 n;		//,i;

#if 0
	i = port->txInIdx + 1;
	if( i >= port->txLen)
	i = 0;

	n = 0;
	while(len-- && i != port->txOutIdx)
	{
		port->txBuf[port->txInIdx] = *d++;
		port->txInIdx = i++;
		if( i >= port->txLen)
		i = 0;
		n++;
	}
#endif
	n = 0;
	while (len-- && port->txInIdx < port->txOutIdx)
	{
		port->txBuf[port->txInIdx++] = *d++;
		n++;
	}
	return n;
}
HCTRL_RET sendPortDat(HCTRL_PTOKEN *token)
{
	s32 n;
	HCTRL_RET retVal = HCTRL_RET_OK;

	//DBG_STR("sendPortDat...\r\n");
	if (token->sendCnts)
	{
		if (token->portObj->txBuf)
		{
			n = putDat2TxBuf(token->portObj, &token->cmdPkt.buf[token->cmdIdx],
					token->sendCnts);
			token->sendCnts -= n;
			token->cmdIdx += n;
			if (token->portObj->outStream)
				token->portObj->outStream(NULL, 0, token->portObj->funcParam);
			if (!n)
				retVal = HCTRL_RET_IDLE;

		}else if (token->portObj->outStream)
		{
			n = token->portObj->outStream(&token->cmdPkt.buf[token->cmdIdx],
					token->sendCnts, token->portObj->funcParam);
			if (n > 0)
			{
				token->cmdIdx += n;
				token->sendCnts -= n;
			}else if (n < 0)
			{		// error
				token->sendCnts = 0;
			}else
			{		// timeout
				retVal = HCTRL_RET_IDLE;
			}
		}else
		{		//error
			token->sendCnts = 0;
		}
	}

	if (!token->sendCnts)
		cmpltToken(token);
	else
		token->tokenProc = sendPortDat;

	return retVal;
}
HCTRL_RET HCTRL_CMD_Ack(HCTRL_PTOKEN *token, u8 errNo)
{
	token->cmdPkt.Len = 0;
	token->cmdPkt.Sum = errNo;
	token->cmdIdx = 0;
	token->sendCnts = 4;

	return sendPortDat(token);
}

static HCTRL_RET hctrl_cmd_nouse(HCTRL_PTOKEN *token)
{
	DBG_STR("hctrl_cmd_nouse...\r\n");
	DBG_HEX(token->cmdPkt.buf, token->cmdIdx);

	clsPortBuf(token->portObj);
	return HCTRL_CMD_Ack(token, HCTRL_ACK_OK);
}
static HCTRL_RET hctrl_cmdGet_model(HCTRL_PTOKEN *token)
{
	token->cmdPkt.Len = DEV_GET_MODEL(token->cmdPkt.dats, HCTRL_CMDPKT_DATLEN,
			SN_ID_Model);
	if (token->cmdPkt.Len)
	{
		mkSendPkt(token);
		return sendPortDat(token);
	}

	return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);
}
static HCTRL_RET hctrl_cmdGet_devNO(HCTRL_PTOKEN *token)
{
	token->cmdPkt.Len = DEV_GET_MODEL(token->cmdPkt.dats, HCTRL_CMDPKT_DATLEN,
			SN_ID_Serial);
	if (token->cmdPkt.Len)
	{
		mkSendPkt(token);
		return sendPortDat(token);
	}

	return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);
}

static HCTRL_RET hctrl_cmdGet_protVer(HCTRL_PTOKEN *token)
{
	token->cmdPkt.dats[0] = (u8) (DSEMUL_PVER >> 8);
	token->cmdPkt.dats[1] = (u8) DSEMUL_PVER;
	token->cmdPkt.dats[2] = (u8) (DSEMUL_DVER >> 8);
	token->cmdPkt.dats[3] = (u8) DSEMUL_DVER;
	token->cmdPkt.Len = 4;

	mkSendPkt(token);

	return sendPortDat(token);
}
static HCTRL_RET hctrl_cmd_connt(HCTRL_PTOKEN *token)
{
	if (!(gDSInfo.emulStat & DSEMUL_STAT_CONNECTED))
	{
		if ((token->cmdPkt.dats[0] == 0xff) && (token->cmdPkt.dats[1] == 0x98)
				&& (token->cmdPkt.dats[3] == 0x76)
				&& (token->cmdPkt.dats[5] == 0x54))
		{
			//gDSInfo.emulStat |= DSEMUL_STAT_CONNECTED;
		}
		else if (DEV_RET_FAIL(DEV_CONNT(token->cmdPkt.dats,token->cmdPkt.Len)))
		{
			return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);
		}
		gDSInfo.emulStat |= DSEMUL_STAT_CONNECTED;
	}

	return HCTRL_CMD_Ack(token, HCTRL_ACK_OK);
}

static HCTRL_RET hctrl_cmdGet_devStat(HCTRL_PTOKEN *token)
{
	DEV_RET n;
	u8 idx = 0xff;

	if (token->cmdPkt.Len)
	{
		idx = token->cmdPkt.dats[0];
	}
	n = DEV_GET_DEVSTAT(idx, token->cmdPkt.dats, HCTRL_CMDPKT_DATLEN);
	if (DEV_RET_FAIL(n))
		return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);

	token->cmdPkt.Len = (u8) n;
	mkSendPkt(token);

	return sendPortDat(token);
}

static HCTRL_RET hctrl_cmdGet_devInfo(HCTRL_PTOKEN *token)
{
	DEV_RET n;
	u8 m = 0xff;

	if (token->cmdPkt.Len)
	{
		m = token->cmdPkt.dats[0];
	}
	n = DEV_GET_DEVINFO(m, token->cmdPkt.dats, HCTRL_CMDPKT_DATLEN);
	if (DEV_RET_FAIL(n))
		return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);

	token->cmdPkt.Len = (u8) n;
	mkSendPkt(token);

	return sendPortDat(token);
}

static HCTRL_RET hctrl_cmd_resetCfg(HCTRL_PTOKEN *token)
{
	if (DEV_RET_FAIL(DEV_RESET_CFGINFO(token->cmdPkt.dats,token->cmdPkt.Len)))
		return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);

	return HCTRL_CMD_Ack(token, HCTRL_ACK_OK);
}

static HCTRL_RET hctrl_cmdGet_statisInfo(HCTRL_PTOKEN *token)
{
	DEV_RET n;

	n = DEV_GET_STATISINFO(token->cmdPkt.dats, HCTRL_CMDPKT_DATLEN);
	if (DEV_RET_FAIL(n))
		return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);

	token->cmdPkt.Len = (u8) n;
	mkSendPkt(token);

	return sendPortDat(token);
}

/*static HCTRL_RET hctrl_cmdGet_maintainInfo(HCTRL_PTOKEN *token)
 {
 return HCTRL_CMD_Ack(token,HCTRL_ACK_OK);
 }*/

static HCTRL_RET hctrl_cmd_devRestart(HCTRL_PTOKEN *token)
{
	u8 idx = 1;

	HCTRL_CMD_Ack(token, HCTRL_ACK_OK);

	if (token->cmdPkt.Len)
	{
		if (token->cmdPkt.dats[0])
			idx = token->cmdPkt.dats[0];
	}

	HCTRL_DELAY_MS(idx * 100);

	DEV_SOFT_RESTART();

	return HCTRL_RET_OK;
}

static HCTRL_RET hctrl_cmdGet_workMode(HCTRL_PTOKEN *token)
{
	DEV_RET n;

	token->cmdPkt.dats[0] = gDSInfo.mode;
	n = DEV_GET_WORKMODE(token->cmdPkt.dats, HCTRL_CMDPKT_DATLEN - 1);
	if (DEV_RET_FAIL(n))
		return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);

	token->cmdPkt.Len = (u8) n + 1;
	mkSendPkt(token);

	return sendPortDat(token);
}

static void HCTRL_Enter_Sample(void)
{

}

static u8 HCTRL_Exit_Sample(u8 *lpDats)
{

	return 8;
}
static HCTRL_RET hctrl_cmdSet_workMode(HCTRL_PTOKEN *token)
{
	u8 newMode, i = 0;

	newMode = token->cmdPkt.dats[0];
	switch (newMode)
	{
	case DSINFO_MODE_NORMAL:
		if (gDSInfo.mode == DSINFO_MODE_SAMPLE)
		{
			i = HCTRL_Exit_Sample(&token->cmdPkt.dats[1]);
		}
		break;
	case DSINFO_MODE_SAMPLE:
		if (gDSInfo.mode == DSINFO_MODE_NORMAL
				|| gDSInfo.mode == DSINFO_MODE_SAMPLE)
		{
			HCTRL_Enter_Sample();
		}
		else
			return HCTRL_CMD_Ack(token, HCTRL_ERR_MODE);

		break;
	case DSINFO_MODE_IDLE:
		break;
		//case DSINFO_MODE_UDF:
		//break;
	case DSINFO_MODE_TEST:
		if (!(gDSInfo.mode == DSINFO_MODE_NORMAL
				|| gDSInfo.mode == DSINFO_MODE_TEST))
		{
			return HCTRL_CMD_Ack(token, HCTRL_ERR_MODE);
		}
		break;
	case DSINFO_MODE_FACTORY:
		break;
	default:
		return HCTRL_CMD_Ack(token, HCTRL_ERR_PARAM);
		break;
	}

	token->cmdPkt.dats[0] = gDSInfo.mode;
	gDSInfo.mode = newMode;
	token->cmdPkt.Len += i;
	mkSendPkt(token);

	return sendPortDat(token);
}

static HCTRL_RET hctrl_cmdGet_verInfo(HCTRL_PTOKEN *token)
{
	DEV_RET n;

	n = DEV_GET_VERINFO(token->cmdPkt.dats, HCTRL_CMDPKT_DATLEN);
	if (DEV_RET_FAIL(n))
		return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);

	token->cmdPkt.Len = (u8) n;
	mkSendPkt(token);

	return sendPortDat(token);
}

static HCTRL_RET hctrl_cmd_DFU(HCTRL_PTOKEN *token)
{

	u8 newMode = 0;

	if (token->cmdPkt.Len)
		newMode = token->cmdPkt.dats[0];

	token->cmdPkt.dats[0] = gDSInfo.mode;
	token->cmdPkt.dats[1] = 0;
	token->cmdPkt.dats[2] = 0;		// * 100ms
	token->cmdPkt.Len = 3;

	gDSInfo.mode = newMode ? DSINFO_MODE_NORMAL : DSINFO_MODE_DFU;

	mkSendPkt(token);

	return sendPortDat(token);
}

static HCTRL_RET hctrl_cmdGet_cfgInfo(HCTRL_PTOKEN *token)
{
	DEV_RET n;
	u16 idx = 0;

	if (token->cmdPkt.Len)
	{
		idx = token->cmdPkt.dats[0];
		if (token->cmdPkt.Len > 1)
			idx = (idx << 8) + token->cmdPkt.dats[1];
	}
	n = DEV_GET_CFGINFO(idx, token->cmdPkt.dats, HCTRL_CMDPKT_DATLEN);
	if (DEV_RET_FAIL(n))
		return HCTRL_CMD_Ack(token,
				n == DEV_ERR_NODAT ? HCTRL_ERR_NODATA : HCTRL_ERR_CMDFAIL);

	token->cmdPkt.Len = (u8) n;
	mkSendPkt(token);

	return sendPortDat(token);
}

static HCTRL_RET hctrl_cmdSet_cfgInfo(HCTRL_PTOKEN *token)
{
	DEV_RET n;
	u16 idx;

	if (token->cmdPkt.Len < 2)
	{
		return HCTRL_CMD_Ack(token, HCTRL_ERR_PARAM);
	}
	else
		idx = ((u16) token->cmdPkt.dats[0] << 8) + token->cmdPkt.dats[1];

	n = DEV_SET_CFGINFO(idx, &token->cmdPkt.dats[2], token->cmdPkt.Len - 2);
	if (DEV_RET_FAIL(n))
		return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);

	token->cmdPkt.Len = 2;
	token->cmdPkt.dats[0] = (n >> 8);
	token->cmdPkt.dats[1] = (u8) n;
	mkSendPkt(token);

	return sendPortDat(token);
}
static HCTRL_RET hctrl_cmdSet_sysParam(HCTRL_PTOKEN *token)
{
	DEV_RET n;

	n = DEV_SET_SYSPARAM(token->cmdPkt.dats, token->cmdPkt.Len);
	if (DEV_RET_FAIL(n))
		return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);

	token->cmdPkt.Len = 1;
	token->cmdPkt.dats[0] = (u8) n;
	mkSendPkt(token);

	return sendPortDat(token);
}
static HCTRL_RET hctrl_cmdGet_sysParam(HCTRL_PTOKEN *token)
{
	DEV_RET n = 0xff;

	if (token->cmdPkt.Len)
		n = token->cmdPkt.dats[0];

	n = DEV_GET_SYSPARAM(n, token->cmdPkt.dats, HCTRL_CMDPKT_DATLEN);
	if (DEV_RET_FAIL(n))
		return HCTRL_CMD_Ack(token, HCTRL_ERR_CMDFAIL);

	token->cmdPkt.Len = (u8) n;
	mkSendPkt(token);

	return sendPortDat(token);
}


static HCTRL_RET cmdPktParse_cnnted(HCTRL_PTOKEN *token)
{
	/*if(!(gDSInfo.emulStat & DSEMUL_STAT_CONNECTED))
	 {
	 return HCTRL_CMD_Ack(token,HCTRL_ERR_UNCONNT);
	 }*/
	switch (token->cmdPkt.Code)
	{
	case CTRL_CMD_UNCONNT:
		gDSInfo.emulStat &= ~DSEMUL_STAT_CONNECTED;
		HCTRL_CMD_Ack(token, HCTRL_ACK_OK);
		break;
		//case CTRL_CMDSET_PSW:
		//hctrl_cmdSet_psw(token);
		//break;
		//case CTRL_CMDGET_USERDAT:
		//hctrl_cmdGet_userDat(token);
		//break;
		//case CTRL_CMDSET_USERDAT:
		//hctrl_cmdSet_userDat(token);
		//break;
	case CTRL_CMDGET_DEVSTAT:
		hctrl_cmdGet_devStat(token);
		break;
		/*case CTRL_CMDGET_PWSSTAT:
		 hctrl_cmdSet_pws(token);
		 break;
		 case CTRL_CMDSET_ENCRYPT:
		 hctrl_cmdSet_encrypt(token);
		 break;*/
	case CTRL_CMDGET_DEVINFO:
		hctrl_cmdGet_devInfo(token);
		break;
	case CTRL_CMD_RESETCFG:
		hctrl_cmd_resetCfg(token);
		break;
		//case CTRL_CMD_CLSBUF:
		//hctrl_cmd_clsBuf(token);
		//	break;
	case CTRL_CMDGET_STATISINFO:
		hctrl_cmdGet_statisInfo(token);
		break;
		//case CTRL_CMDGET_MAINTAININFO:
		//hctrl_cmdGet_maintainInfo(token);
		//break;
	case CTRL_CMD_DEVRESTART:
		hctrl_cmd_devRestart(token);
		break;
		//case CTRL_CMD_DEVCHKSLF:
		//hctrl_cmd_devChkSlf(token);
		//break;
	case CTRL_CMDGET_WORKMODE:
		hctrl_cmdGet_workMode(token);
		break;
	case CTRL_CMDSET_WORKMODE:
		hctrl_cmdSet_workMode(token);
		break;
	case CTRL_CMDGET_VERINFO:
		hctrl_cmdGet_verInfo(token);
		break;
	case CTRL_CMD_DFUMODE:
		hctrl_cmd_DFU(token);
		break;
	case CTRL_CMDGET_CGFINFOS:
		hctrl_cmdGet_cfgInfo(token);
		break;
	case CTRL_CMDSET_CFGINFOS:
		hctrl_cmdSet_cfgInfo(token);
		break;
//	case CTRL_CMDGET_CFGFMT:
		//	hctrl_cmdGet_cfgFmt(token);
		//break;
	case CTRL_CMDSET_SYSPARAM:
		hctrl_cmdSet_sysParam(token);
		break;
	case CTRL_CMDGET_SYSPARAM:
		hctrl_cmdGet_sysParam(token);
		break;

	default:
		HCTRL_CMD_Ack(token, HCTRL_ERR_UNSUPPORT);
		break;
	}

	return HCTRL_RET_OK;
}
static HCTRL_RET dfuParse(HCTRL_PTOKEN *token)
{
	u8 ack;

	//DBG_TRACE("token->cmdPkt.Code=%#x\r\n",token->cmdPkt.Code);

	switch (token->cmdPkt.Code)
	{
	case CTRL_CMD_NOUSE:
		hctrl_cmd_nouse(token);
		break;
	case CTRL_CMDGET_MODEL:
		hctrl_cmdGet_model(token);
		break;
	case CTRL_CMDGET_DEVNO:
		hctrl_cmdGet_devNO(token);
		break;
	case CTRL_CMD_DFUMODE:
		hctrl_cmd_DFU(token);
		break;
	case CTRL_CMD_CONNT:
	case CTRL_CMD_UNCONNT:
		 HCTRL_CMD_Ack(token, HCTRL_ACK_OK);
		break;
	case CTRL_CMD_DEVRESTART:
		hctrl_cmd_devRestart(token);
		break;
	case CTRL_CMDGET_WORKMODE:
		hctrl_cmdGet_workMode(token);
		break;
	case CTRL_CMDGET_VERINFO:
		hctrl_cmdGet_verInfo(token);
		break;
	case CTRL_DFU_START:
	case CTRL_DFU_GOING:
	case CTRL_DFU_FINISH:
	case CTRL_DFU_CANCEL:
		if (token->cmdIdx > 4)
			ack = dfu_proc(token->cmdPkt.Code, &token->cmdPkt.buf[4],
					token->cmdIdx - 4);
		else
			ack = dfu_proc(token->cmdPkt.Code,
			NULL, 0);

		HCTRL_CMD_Ack(token, ack);
		break;
	default:
		HCTRL_CMD_Ack(token, HCTRL_ERR_UNSUPPORT);
		break;
	}
	return HCTRL_RET_OK;
}

//HCTRL_RET hctrl_cmd_test(HCTRL_PTOKEN *token);
static HCTRL_RET cmdPktParse(HCTRL_PTOKEN *token)
{
//	DBG_STR("cmdPktParse...\r\n");
	if (gDSInfo.mode == DSINFO_MODE_SAMPLE)
	{
		if (token->cmdPkt.Code == CTRL_CMDSET_WORKMODE)
			return hctrl_cmdSet_workMode(token);

		return HCTRL_CMD_Ack(token, HCTRL_ERR_MODE);
	}
	else if (gDSInfo.mode == DSINFO_MODE_TEST)
	{
		/*if(token->cmdPkt.Code == CTRL_CMDSET_WORKMODE)
		 return hctrl_cmdSet_workMode(token);
		 else if(token->cmdPkt.Code == CTRL_CMD_TEST)
		 return hctrl_cmd_test(token);
		 */
		return HCTRL_CMD_Ack(token, HCTRL_ERR_MODE);
	}
	else if (gDSInfo.mode == DSINFO_MODE_DFU)
	{
		return dfuParse(token);
	}

	//DBG_TRACE("cmdPktParse code = 0x%x ... \r\n",token->cmdPkt.Code);

	switch (token->cmdPkt.Code)
	{
	case CTRL_CMD_NOUSE:
		hctrl_cmd_nouse(token);
		break;
	case CTRL_CMDGET_MODEL:
		hctrl_cmdGet_model(token);
		break;
	case CTRL_CMDGET_DEVNO:
		hctrl_cmdGet_devNO(token);
		break;
	case CTRL_CMDGET_PROTVER:
		hctrl_cmdGet_protVer(token);
		break;
	case CTRL_CMD_CONNT:
		hctrl_cmd_connt(token);
		break;
		//case CTRL_CMD_TEST:
		//hctrl_cmd_test(token);
		//break;
	default:
		return cmdPktParse_cnnted(token);
		break;
	}

	return HCTRL_RET_OK;
}

static HCTRL_RET getCtrlPkt(HCTRL_PTOKEN *token)
{
	s16 d;
	u8 m;

//	DBG_STR("getCtrlPkt...\r\n");

	do
	{
		if (gDSInfo.mode == DSINFO_MODE_SAMPLE)
		{
			if (token->portObj->outStream && HCTRL_TXBUF_UNEMPTY(token->portObj))
			{
				token->portObj->outStream(NULL, 0, token->portObj->funcParam);
			}
		}
		d = getPortDat(token->portObj);
		if (d < 0)
		{
			if (!token->cmdIdx)// && (gDSInfo.mode == DSINFO_MODE_NORMAL))
				resetToken(token);

			return HCTRL_RET_IDLE;
			//break;
		}
		if (!token->cmdIdx)
		{
			if ((u8) d != HCTRL_PKT_ID)
				continue;
		}

		token->cmdPkt.buf[token->cmdIdx++] = (u8) d;
		if ((token->cmdIdx < HCTRL_CMDPKT_HDLEN)
				|| (token->cmdIdx < (HCTRL_CMDPKT_HDLEN + token->cmdPkt.Len)))
		{
			token->stat = HCTRL_TOKENSTAT_WAITING;

			continue;
		}
		m = 0;
		for (d = 0; d < token->cmdPkt.Len; d++)
			m += token->cmdPkt.dats[d];
		if (m == token->cmdPkt.Sum)
		{
			token->stat = HCTRL_TOKENSTAT_PARSE;

			token->tokenProc = cmdPktParse;
			cmdPktParse(token);
			//break;
		}else
		{
			HCTRL_CMD_Ack(token, HCTRL_ERR_SUM);
		}
		break;
	} while (1);

	return HCTRL_RET_OK;
}
/*
static HCTRL_PORT* getCtrlPortObj(void)
{
	u8 i;

	for (i = 0; i < HCTRL_PORT_NUMS && (gCtrlPort[i].type & HCTRL_PTYPE_USED);
			i++)
	{
		//if(gCtrlPort[i].rsInIdx != gCtrlPort[i].rsOutIdx)
		return &gCtrlPort[i];
	}
	return NULL;
}
*/
HCTRL_PTOKEN *getCtrlToken(u8 handle)
{
	return  (handle < HCTRL_TOKEN_NUMS) ?
			&gCtrlToken[handle] :
			NULL;

}
u8 getCtrlTokenStat(u8 handle)
{
	return  (handle < HCTRL_TOKEN_NUMS) ?
			gCtrlToken[handle].stat :
			0xff;
}
HCTRL_RET hCtrlProtoProc(u8 handle)
{
	//DBG_STR("ctrlProtoProc ... \r\n");

	HCTRL_RET retVal = HCTRL_RET_IDLE;

	if (gCtrlToken[handle].tokenProc)
	{
		retVal = gCtrlToken[handle].tokenProc(&gCtrlToken[handle]);
	}else
	{
		//if (!gCtrlToken[handle].portObj)
		{
			//gCtrlToken[handle].portObj = getCtrlPortObj();
			if (!gCtrlToken[handle].portObj)
				return HCTRL_RET_IDLE;
		}

		gCtrlToken[handle].cmdIdx = 0;
		gCtrlToken[handle].tokenProc = getCtrlPkt;
		retVal = getCtrlPkt(&gCtrlToken[handle]);
	}

	return retVal;
}

void initCtrlProto(void)
{
	//memset(gCtrlPort, 0, sizeof(gCtrlPort));
	memset(gCtrlToken, 0, sizeof(gCtrlToken));

	memset(&gDSInfo, 0, sizeof(gDSInfo));
	gDSInfo.mode = DSINFO_MODE_NORMAL;

	DEV_CTRL_INIT();

	//memset(&gDSSmpl,0,sizeof(gDSSmpl));
}

void initCtrlPort(HCTRL_PORT* lpPort,u8 *rsBuf,u8 *txBuf,HCTRL_OUTSTREAM outFunc,void *param)
{
	lpPort->rsBuf = rsBuf;
	lpPort->txBuf = txBuf;
	lpPort->outStream = outFunc;
	lpPort->funcParam = param;
	lpPort->rsInIdx = lpPort->rsOutIdx = 0;
	lpPort->txInIdx = lpPort->txOutIdx = 0;
	lpPort->type = 0;

}
HCTRL_PHANDLE regCtrlPort(HCTRL_PORT* lpPort)
{
	u8 i;

	if(!lpPort)
		return HCTRL_PHANDLE_INVALID;

	for (i = 0; i < HCTRL_TOKEN_NUMS; i++)
	{
		if (!(gCtrlToken[i].tag & HCTRL_TOKEN_INUSE))
		{
			gCtrlToken[i].portObj = lpPort;
			gCtrlToken[i].tag |= HCTRL_TOKEN_INUSE;
			return i;
		}
	}
	return HCTRL_PHANDLE_INVALID;
}
u8 unRegCtrlPort(u8 handle)
{
	if(handle < HCTRL_TOKEN_NUMS)
	{
		gCtrlToken[handle].tag &= ~HCTRL_TOKEN_INUSE;
		return 1;
	}
	return 0;
}
HCTRL_PHANDLE hCtrl_Attach(HCTRL_PHANDLE handle, u8 *rsBuf, u16 rsDats,
		u8* txBuf, u16 txBufLen)
{
	HCTRL_PORT* lpObj;// = getCtrlPortObj();

	if (!(handle < HCTRL_TOKEN_NUMS))
		return -1;

	if(!(gCtrlToken[handle].tag & HCTRL_TOKEN_INUSE))
		return -2;

	lpObj = gCtrlToken[handle].portObj;
	if(!lpObj)
		return -3;

	if (rsBuf)
	{
		lpObj->rsBuf = rsBuf;
		lpObj->rsInIdx = rsDats;
		lpObj->rsOutIdx = 0;
		gCtrlToken[handle].stat = HCTRL_TOKENSTAT_START;
	}

	if (txBuf)
	{
		lpObj->txBuf = txBuf;
		lpObj->txInIdx = txBufLen;
		lpObj->txOutIdx = 0;
	}

	return HCTRL_RET_OK;
}
HCTRL_RET searchCtrlPkt(HCTRL_PTOKEN *token)
{
	s16 d;
	u8 m;

	//DBG_TRACE("searchCtrlPkt(%d,%d)...\r\n",token->portObj->rsInIdx,token->portObj->rsOutIdx);

	do
	{
		d = getPortDat(token->portObj);
		if (d < 0)
		{
			if (!token->cmdIdx)
				resetToken(token);

			return HCTRL_RET_IDLE;
		}
		if (!token->cmdIdx)
		{
			if ((u8) d != HCTRL_PKT_ID)
				continue;
		}

		token->cmdPkt.buf[token->cmdIdx++] = (u8) d;
		//if ((token->cmdIdx < HCTRL_CMDPKT_HDLEN)
			//	|| (token->cmdIdx < (HCTRL_CMDPKT_HDLEN + token->cmdPkt.Len)))
		if(token->cmdIdx < (HCTRL_CMDPKT_HDLEN + token->cmdPkt.Len))
		{
			token->stat = HCTRL_TOKENSTAT_WAITING;
			continue;
		}
		m = 0;
		for (d = 0; d < token->cmdPkt.Len; d++)
			m += token->cmdPkt.dats[d];

		if (m == token->cmdPkt.Sum)
			break;

		//DBG_TRACE("searchCtrlPkt : Sum error(%d,%d)...\r\n",m,token->cmdPkt.Sum);
	} while (1);

	token->cmdIdx = 0;
	return HCTRL_RET_OK;
}
