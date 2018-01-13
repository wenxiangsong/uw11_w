/*
 * app_netmsg.h
 *
 *  Created on: Dec 7, 2017
 *      Author: root
 */

#ifndef APP_NETMSG_H_
#define APP_NETMSG_H_

#include <qcom/qcom_common.h>
#include "HCtrlProto.h"

#define NETMSG_ID_0		0x40
#define NETMSG_ID_1		0x41
#define NETMSG_ID_2		0x2F
#define NETMSG_ID_3		0x3F
#define NETMSG_ID		((NETMSG_ID_3 << 24) + (NETMSG_ID_2 << 16) + (NETMSG_ID_1 << 8) + (NETMSG_ID_0))

#define NET_MSGHD_SIZE	sizeof(MSG_HD)

#define NETMSG_VER		0x0001

//---------CODE_HI------------
#define NETMSG_CODE_MODE(c)	((c) >> 8)
#define	CODE_MODE_HOST		0
#define CODE_MODE_DEV		1
#define CODE_MODE_SVR		2

//--------CODE_LO-------------
#define NETMSG_CODE_TYPE(c)	((A_UINT8)(c))
#define CODE_TYPE_LOGIN		0x01
#define CODE_TYPE_DSEMUL	0x05
#define CODE_TYPE_DATS		0x09

#define NETMSG_SETCODE(m,t) ((A_UINT16)((((m) & 0xff) << 8) + ((t) & 0xff)))
//---------errcode------------------
#define NETMSG_ERR_NONE			0x00
#define NETMSG_ERR_NOLOGIN		0x0001
#define NETMSG_ERR_UNSUPPORT	0x0002
typedef union _NETMSG_HD{
	struct
	{
	A_UINT32 msgID;
	A_UINT32 msgNO;
	A_UINT32 len;
	A_UINT16 code;
	A_UINT16 Ver;
	A_UINT16 errcode;
	A_UINT8 resv[2];
	};
	A_UINT8	buf[1];

}MSG_HD;

struct _NETMSG_PKG ;
typedef A_INT32 (*NETMSG_FUNC)(struct _NETMSG_PKG *lpMsg);

//----------stat---------------
#define NETMSG_STAT_IDLE		0
#define NETMSG_STAT_MATCH		1	//search ID
#define NETMSG_STAT_PARSE		2	//
#define NETMSG_STAT_FINISH		3

//---------tag---------------------
#define NETMSG_TAG_NONE			0x00
#define NETMSG_TAG_LOGIN		0x01
typedef struct _NETMSG_PKG{
	NETMSG_FUNC	msgProc;

	MSG_HD hdPkt;
	A_UINT32 recvPktIdx;

	A_UINT8 *lpDats;
	A_UINT32 datLen;

	HCTRL_PHANDLE ctrlHandle;

	A_UINT32 timer;

	A_UINT8	tag;	//
	A_UINT8	stat;

}NET_MSG;

extern const MSG_HD gMsgHD_Def;
void netMsg_reset(NET_MSG* lpMsg);
void netMsg_init(NET_MSG *lpMsg);
A_INT32 netMsg_proc(NET_MSG *lpMsg);

#endif /* APP_NETMSG_H_ */
