/*
 * app_tcp.c
 *
 *  Created on: Dec 2, 2017
 *      Author: root
 */
#include "qcom_common.h"
#include "app_common.h"
#include "qcom/socket_api.h"
#include "qcom/qcom_set_Nf.h"
#include "hwcrypto_api.h"
#include "malloc_api.h"
#include "qcom/qcom_gpio.h"
#include "ezxml.h"
#include "app_main.h"
#include "qcom/tx_alloc_api.h"
#include "qcom/select_api.h"
#include "qcom/qcom_ssl.h"
#include "paramconfig.h"
#include "app_netmsg.h"
#include "app_dev.h"
#include "app_led.h"

#pragma pack(4)
static A_UINT8 tcpCtrl_send_buf[1024],tcpCtrl_recv_buf[2048];
static A_UINT8 tcpData_send_buf[1024],tcpData_recv_buf[2048];

#pragma   pack()

extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void swat_bench_tcp_tx(A_UINT32 index);
extern void swat_bench_udp_tx(A_UINT32 index);
extern int get_empty_task_index(void);
A_INT32 syncSendDevPkt(HCTRL_CMDPKT *lpPkt);
/*********************************************Tcp Service**************************************************************************************************/
extern void swat_bench_pwrmode_maxperf();
extern void swat_bench_restore_pwrmode();
typedef struct _HCTRL_TCP_PARAM
{
	NET_MSG *lpMsg;
	STREAM_CXT_t *pStreamCxt;
	u8 clientIdx;
}HCTRL_TCP_PARAM;

#define NETMSG_TCP_TIMEOUT	5000
static s32 tcp_NetMsg_rspPkt(u8 *lpDat,u16 dLen,void *p)
{
	A_UINT32 bufLen;
	A_UINT8 *lpSendBuf;
	A_INT32 sendBytes ;
	HCTRL_TCP_PARAM *lpParam = (HCTRL_TCP_PARAM*)p;
	MSG_HD *lpMsgHd ;

	DBG_STR("tcp_NetMsg_rspPkt : \r\n");

	if(lpParam->pStreamCxt->index == TCP_CFGPARAM_IDX)
	{
		lpSendBuf = tcpCtrl_send_buf;
		bufLen = sizeof(tcpCtrl_send_buf);
	}else
	{
		lpSendBuf = tcpData_send_buf;
		bufLen = sizeof(tcpData_send_buf);
	}

	lpMsgHd = (MSG_HD*)lpSendBuf;//tcpCtrl_send_buf;
	*lpMsgHd = lpParam->lpMsg->hdPkt;

	if(lpMsgHd->errcode == NETMSG_ERR_NONE)
	{
		if(dLen > (bufLen - NET_MSGHD_SIZE))
		{
			DBG_TRACE("tcp_NetMsg_rspPkt : Send Datas too more(%d)...\r\n",dLen);
			return -1;
		}

		lpMsgHd->len = dLen;
		if(dLen && lpDat)
			memcpy(&lpSendBuf[NET_MSGHD_SIZE],lpDat,dLen);
	}else
	{
		dLen = 0;
		DBG_TRACE("tcp_NetMsg_rspPkt : errcode(%d)\r\n",lpMsgHd->errcode);
	}
	sendBytes = swat_send(lpParam->pStreamCxt->clientFd[lpParam->clientIdx],
				(char *)lpSendBuf,dLen + NET_MSGHD_SIZE,0);

	return lpMsgHd->errcode ? -2 : (sendBytes - NET_MSGHD_SIZE);
}

A_UINT16 tcp_get_netMsg_Port(void *lp)
{
	HCTRL_TCP_PARAM *lpParam = (HCTRL_TCP_PARAM*)lp;
	A_UINT16 p = 0;

	if( lpParam->pStreamCxt->index == TCP_CFGPARAM_IDX )
		p = CFG_PORT;
	else if( lpParam->pStreamCxt->index == TCP_DATPARAM_IDX)
		p = DATA_PORT;
	return p;
}

//------------------------client----------------------------------
void setClientLogin(NET_MSG  *lpMsg)
{
	HCTRL_PTOKEN *lpToken = getCtrlToken(lpMsg->ctrlHandle);
	HCTRL_TCP_PARAM *lpP = (HCTRL_TCP_PARAM *)lpToken->portObj->funcParam;
	STREAM_CXT_t * pStreamCxt = lpP->pStreamCxt;
	DBG_STR("set Client Login\r\n" );
	if(pStreamCxt->param.procStat == PLOGIN_STAT)
	{
		pStreamCxt->param.procStat = PTRAN_STAT;
		lpMsg->tag |= NETMSG_TAG_LOGIN;

		DBG_STR("setClientLogin : ok \r\n" );
	}
}

static DEV_REQPKT gReqPkt[] =
{
		{CTRL_CMD_CONNT,0},				//dev connect
		{CTRL_CMDGET_DEVINFO,1,{0,0}},	//get dev info
};
A_INT16 getDevRspInfo(A_UINT8 hCtrl,DEV_REQPKT *lpReq,A_UINT8 *lpRsqBuf,A_UINT16 *bufLen);
static A_UINT8 tcp_client_login(HCTRL_TCP_PARAM *param)
{
	STREAM_CXT_t * pStreamCxt = param->pStreamCxt;
	MSG_HD *lpNetMsg = (MSG_HD*)tcpCtrl_send_buf;

	A_INT32 m;
	A_UINT16 bufIdx,n;
	A_INT16 ret;
	A_UINT8 i,hctrl;

	hctrl = param->lpMsg->ctrlHandle;

	*lpNetMsg = gMsgHD_Def;
	bufIdx = NET_MSGHD_SIZE;
	ret = 0;
	for( i = 0; i < sizeof(gReqPkt) / sizeof(*gReqPkt);i++)
	{
		n = sizeof(tcpCtrl_send_buf) - bufIdx;
		ret = getDevRspInfo(hctrl,&gReqPkt[i],&tcpCtrl_send_buf[bufIdx],&n);
		if(ret)
		{
			DBG_TRACE("tcp_client_login : get device info fail(%d)\r\n",ret);
			break;
		}
		if(gReqPkt[i].code == CTRL_CMDGET_DEVINFO)
		{
			lpNetMsg->code = NETMSG_SETCODE(CODE_MODE_SVR,CODE_TYPE_LOGIN);
			lpNetMsg->len = (A_UINT32)n;

			m = n + NET_MSGHD_SIZE;
			if(m == qcom_send(pStreamCxt->socketLocal,(char*)tcpCtrl_send_buf,m,0))
			{
				DBG_TRACE("tcp_client_login : qcom_send ok(%d)...\r\n",m);
				ret = 1;
			}
		}
	}

	return ret;
}
static void swat_tcp_client_data_handle(HCTRL_TCP_PARAM *param)
{
	STREAM_CXT_t * pStreamCxt = param->pStreamCxt;
	NET_MSG *lpNetMsg = param->lpMsg;
	A_INT32 ret = 0, recvBytes = 0;
	q_fd_set socket;//,exSkt;
	struct timeval tmo;

	param->clientIdx = 0;
	pStreamCxt->clientFd[0] = pStreamCxt->socketLocal;
	lpNetMsg->datLen = 0;

	tmo.tv_sec = 2;
	tmo.tv_usec = 0;//500;

	FD_ZERO(&socket);
	while(g_state_data.WifiConnected == CONNTED)
	{
		if (swat_bench_quit())
			break;
		if(lpNetMsg->datLen)
		{
			if( netMsg_proc(lpNetMsg) < 0)
			{
				DBG_STR("swat_tcp_client_data_handle : netMsg_proc Fail \r\n");
				lpNetMsg->datLen = 0;
			}
		}
		FD_SET(pStreamCxt->socketLocal, &socket);
		ret = qcom_select(pStreamCxt->socketLocal + 1, &socket, NULL, NULL, &tmo);
		if (ret < 0)
		{
			DBG_STR("tcp_client_data_handle : select ERROR\r\n");
			break;
		} else if (ret == 0)
		{
			tx_thread_sleep(1);
			continue;
		}
		if (FD_ISSET(pStreamCxt->socketLocal, &socket)&& (pStreamCxt->socketLocal > 0))
		{
			recvBytes = swat_recv(pStreamCxt->socketLocal,(char *)pStreamCxt->param.pktBuff,pStreamCxt->param.pktSize,0);
			if (recvBytes < 0)
			{
				DBG_TRACE("swat_tcp_client_data_handle : recv fail(%d)\r\n",recvBytes);
			} else if (recvBytes == 0)
			{
				DBG_STR("swat_tcp_client_data_handle : Socket Close By Server..\n");
				pStreamCxt->param.procStat = PINIT_STAT;
				break;
			} else
			{
				lpNetMsg->datLen = recvBytes;
				lpNetMsg->lpDats = pStreamCxt->param.pktBuff;
				lpNetMsg->timer = time_ms();				
			}

			swat_fd_clr(pStreamCxt->socketLocal, &socket);
		}
	}
	DBG_STR("swat_tcp_client_data_handle : exit ... \r\n");
}

static void swat_tcp_client_proc(STREAM_CXT_t * pStreamCxt)
{
	A_INT32 ret = 0;
	struct sockaddr_in remoteAddr;
	struct sockaddr_in6 remote6Addr;

	NET_MSG mNetMsg;
	HCTRL_TCP_PARAM param;

	param.pStreamCxt = pStreamCxt;
	param.lpMsg = &mNetMsg;

	HCTRL_PORT hPort;

	initCtrlPort(&hPort,NULL,NULL,tcp_NetMsg_rspPkt,(void*)&param);
	HCTRL_PHANDLE hCtrlHandle = regCtrlPort(&hPort);

	netMsg_init(&mNetMsg);
	mNetMsg.ctrlHandle = hCtrlHandle;

	if (pStreamCxt->param.is_v6_enabled)
	{
		swat_mem_set(&remote6Addr, 0, sizeof(struct sockaddr_in6));
		memcpy(&remote6Addr.sin6_addr, pStreamCxt->param.ip6Address.addr,sizeof(IP6_ADDR_T));
		remote6Addr.sin6_port = htons(pStreamCxt->param.port);
		remote6Addr.sin6_family = AF_INET6;
	}else
	{
		swat_mem_set(&remoteAddr, 0, sizeof(struct sockaddr_in));
		remoteAddr.sin_addr.s_addr = htonl(pStreamCxt->param.ipAddress);
		remoteAddr.sin_port = htons(pStreamCxt->param.port);
		remoteAddr.sin_family = AF_INET;
	}
	qcom_tcp_conn_timeout(5);
	while(!swat_bench_quit())
	{
		if(g_state_data.WifiConnected != CONNTED)
		{
			tx_thread_sleep(10);
			pStreamCxt->param.procStat = PINIT_STAT;
			continue;
		}
		switch(pStreamCxt->param.procStat)
		{
			case PLOGOUT_STAT:
			case PINIT_STAT:
				 DBG_STR("swat_tcp_client_proc : PDISCON_STAT\r\n");
				 if (pStreamCxt->socketLocal >= 0)
					 qcom_socket_close(pStreamCxt->socketLocal);	
				 pStreamCxt->socketLocal = swat_socket(AF_INET, SOCK_STREAM, 0);
				 if (pStreamCxt->socketLocal < 0)
				 {
					DBG_STR("Open socket error...\r\n");
					break;
				 }
			case PDISCON_STAT:
				if (pStreamCxt->param.is_v6_enabled)
				{
					ret = qcom_connect(pStreamCxt->socketLocal, (struct sockaddr *) &remote6Addr,sizeof (struct sockaddr_in6));
				}else
				{
					ret = qcom_connect(pStreamCxt->socketLocal, (struct sockaddr *) &remoteAddr,sizeof (struct sockaddr_in));
				}
				if( ret < 0)
				{
					DBG_TRACE("swat_tcp_client_data_handle : connect fail(%d)\r\n",ret);
					qcom_thread_msleep(500);
					pStreamCxt->param.procStat = PINIT_STAT;
				}else
				{
					pStreamCxt->param.procStat = PCONN_STAT;
					netMsg_reset(&mNetMsg);
				}
				break;
			case PCONN_STAT:
				if(tcp_client_login(&param))
					pStreamCxt->param.procStat = PLOGIN_STAT;
				else
					qcom_thread_msleep(100);
				break;
			case PLOGIN_STAT:
			case PTRAN_STAT:
				swat_tcp_client_data_handle(&param);
				break;
			default:
				break;
			}
	}
	unRegCtrlPort(hCtrlHandle);
	DBG_STR("swat_tcp_client_proc : exited....\r\n");
}
//------------------------------------------------------------------
static void swat_tcp_service_data_handle(STREAM_CXT_t * pStreamCxt)
{
	A_INT32 ret = 0;
	A_INT32 i = 0;
	struct sockaddr_in clientAddr;
	A_INT32 len;
	struct timeval tmo;
	A_INT32 fdAct = 0;
	q_fd_set sockSet, master;
	A_INT32 recvBytes = 0;
	//A_UINT8 msleep;

//-----------------------------------------------
	NET_MSG mNetMsg;
	HCTRL_TCP_PARAM param;

	param.pStreamCxt = pStreamCxt;
	param.lpMsg = &mNetMsg;

	HCTRL_PORT hPort;

	initCtrlPort(&hPort,NULL,NULL,tcp_NetMsg_rspPkt,(void*)&param);
	HCTRL_PHANDLE hCtrlHandle = regCtrlPort(&hPort);

	netMsg_init(&mNetMsg);
	mNetMsg.ctrlHandle = hCtrlHandle;

//-----------------------------------------
	SWAT_PTR_NULL_CHK(pStreamCxt);
	/* Initial Bench Value */
	swat_bench_quit_init();
	pStreamCxt->pfd_set = (void *) &sockSet;
	tmo.tv_sec = 2;
	tmo.tv_usec = 0;
	/*Init all the client fds*/
	for (i = 0; i < MAX_SOCLIENT; i++)
	{
		pStreamCxt->clientFd[i] = -1;
	}
	ret = swat_listen(pStreamCxt->socketLocal, MAX_SOCLIENT);//10);
	DBG_TRACE("Listening on socket %d,ret = %d\r\n", pStreamCxt->index,ret);//socketLocal);
	while (1)
	{
		/*Start select*/
		while (1)
		{
			if(swat_bench_quit())
				goto QUIT;

			if(mNetMsg.datLen)
			{
				if( netMsg_proc(&mNetMsg) < 0)
				{
					DBG_STR("swat_tcp_service_data_handle : netMsg_proc Fail \r\n");
					mNetMsg.datLen = 0;
				}
			}
			/* Find the master list everytime before calling select() because select* modifies sockSet */
			swat_fd_zero(&master);
			swat_fd_set(pStreamCxt->socketLocal, &master);
			for (i = 0; i < MAX_SOCLIENT; i++)
			{
				if (pStreamCxt->clientFd[i] != -1)
				{
					/*if sockets have been closed in fw, clear the clientfd*/
					if ((swat_fd_isset(pStreamCxt->clientFd[i], &master) == -1))
					{
						swat_close(pStreamCxt->clientFd[i]);
						pStreamCxt->clientFd[i] = -1;
						DBG_STR("Waiting..\n");
					}else
					{
						swat_fd_set(pStreamCxt->clientFd[i], &master);
						break;
					}
				}
			}
			sockSet = master;
			fdAct = swat_select(pStreamCxt->socketLocal, &sockSet, NULL, NULL, &tmo); //k_select()
			if (fdAct != 0)
			{
				break;
			} else
			{
				tx_thread_sleep(1);
				continue;
			}
		}
		/*client socket select*/
		/*receive data first in each client fd*/
		for (i = 0; i < MAX_SOCLIENT; i++)
		{
			if ((pStreamCxt->clientFd[i] != -1)&& swat_fd_isset(pStreamCxt->clientFd[i], &sockSet))
			{
				recvBytes = swat_recv(pStreamCxt->clientFd[i],(char *)pStreamCxt->param.pktBuff,pStreamCxt->param.pktSize,0);
				if (recvBytes < 0)
				{
					SWAT_PTF("ERROR..\n");
				} else if (!recvBytes )
				{
					SWAT_PTF("Socket Sevice Close By Clicent..\n");
					swat_close(pStreamCxt->clientFd[i]);
					pStreamCxt->clientFd[i] = -1;
				} else if (recvBytes)
				{					
					SWAT_PTF("recvBytes:%d\n",recvBytes);
					app_led_flash_on(10);
					mNetMsg.datLen = recvBytes;
					mNetMsg.lpDats = pStreamCxt->param.pktBuff;
					param.clientIdx = i;
					mNetMsg.timer = time_ms();
				}
			}
			swat_fd_clr(pStreamCxt->clientFd[i], &sockSet);
		}
		/*Local socket select*/
		if (swat_fd_isset(pStreamCxt->socketLocal, &sockSet))
		{
			A_INT32 tmpSocket = swat_accept(pStreamCxt->socketLocal, (struct sockaddr *) &clientAddr, &len);
			if(tmpSocket < 0)
				goto QUIT1;
			/*find available socket*/
			for (i = 0; i < MAX_SOCLIENT; i++)
			{
				if (-1 == pStreamCxt->clientFd[i])
					break;
			}
			if (MAX_SOCLIENT == i)
			{
				/*Wait till there is an available socket */
				SWAT_PTF("Failed to accept more than %d socket.\n",MAX_SOCLIENT);
				swat_close(tmpSocket);
				goto QUIT1;
			}else
			{
				pStreamCxt->clientFd[i] = tmpSocket;
				netMsg_reset(&mNetMsg);
			}
	QUIT1:
			swat_fd_clr(pStreamCxt->socketLocal, &sockSet);
		}
	}
QUIT:
	unRegCtrlPort(hCtrlHandle);
	for (i = 0; i < MAX_SOCLIENT; i++)
	{
		if (pStreamCxt->clientFd[i] != -1)
		{
			swat_close(pStreamCxt->clientFd[i]);
			pStreamCxt->clientFd[i] = -1;

		}
	}

	DBG_STR("swat_tcp_service_data_handle : exited....\r\n");
	/* Init fd_set */
	swat_fd_zero(&sockSet);
	/* Close Client Socket */
	pStreamCxt->pfd_set = NULL;
}

static void tcp_transfer_task(A_UINT32 index)
{
	A_INT32 ret = 0;
	STREAM_CXT_t * pStreamCxt = &cxtTcpRxPara[index & 0xff];
	struct sockaddr_in remoteAddr;
	struct sockaddr_in6 remote6Addr;
	swat_bench_pwrmode_maxperf();
	SWAT_PTR_NULL_CHK(pStreamCxt);
	if( pStreamCxt->param.devMode == SERVICE_MODE)
	{
		if (pStreamCxt->param.is_v6_enabled)
		{
			pStreamCxt->socketLocal = swat_socket(AF_INET6, SOCK_STREAM, 0);
			if (pStreamCxt->socketLocal < 0)
			{
				DBG_STR("Open socket error...\r\n");
				goto QUIT;
			}
			swat_mem_set(&remote6Addr, 0, sizeof(struct sockaddr_in6));
			memcpy(&remote6Addr.sin6_addr, pStreamCxt->param.ip6Address.addr,sizeof(IP6_ADDR_T));
			remote6Addr.sin6_port = htons(pStreamCxt->param.port);
			remote6Addr.sin6_family = AF_INET6;
			ret = swat_bind(pStreamCxt->socketLocal, (struct sockaddr *) &remote6Addr,sizeof (struct sockaddr_in6));
		} else
		{
			pStreamCxt->socketLocal = swat_socket(AF_INET, SOCK_STREAM, 0);
			if (pStreamCxt->socketLocal < 0)
			{
				DBG_STR("Open socket error...\r\n");
				goto QUIT;
			}
			/* Connect Socket */
			swat_mem_set(&remoteAddr, 0, sizeof(struct sockaddr_in));
			remoteAddr.sin_addr.s_addr = htonl(pStreamCxt->param.ipAddress);
			remoteAddr.sin_port = htons(pStreamCxt->param.port);
			remoteAddr.sin_family = AF_INET;
			ret = swat_bind(pStreamCxt->socketLocal, (struct sockaddr *) &remoteAddr,sizeof (struct sockaddr_in));
		}
		if (ret < 0)
		{
			DBG_TRACE("tcp_transfer_task : Bind Failed %d.\n", pStreamCxt->index);
			swat_socket_close(pStreamCxt);
			goto QUIT;
		}
	}
	DBG_TRACE("Local port %d ,index(%d): Waiting Connect!\n",pStreamCxt->param.port,pStreamCxt->index);
	/* Packet Handle */
	if (pStreamCxt->index == TCP_DATPARAM_IDX)
	{
		g_state_data.DataSocketConnected = CONNTED;
		DBG_STR("Data Socket Connect Complete!\r\n");
	}
	if (pStreamCxt->index == TCP_CFGPARAM_IDX)
	{
		g_state_data.ConfigSocketConnected = CONNTED;
		DBG_STR("Config Socket Connect Complete!\r\n");
	}
	if(pStreamCxt->param.devMode == SERVICE_MODE)
		swat_tcp_service_data_handle(pStreamCxt);
	else
		swat_tcp_client_proc(pStreamCxt);
QUIT:
	//swat_bench_restore_pwrmode();
	/* Free Index */
	swat_socket_close(pStreamCxt);
	swat_cxt_index_free(&tcpRxIndex[pStreamCxt->index]);
	swat_bench_restore_pwrmode();
	/* Thread Delete */
	swat_task_delete();
	if (pStreamCxt->index == TCP_DATPARAM_IDX)
		g_state_data.DataSocketConnected = UNCONNT;
	if (pStreamCxt->index == TCP_CFGPARAM_IDX)
	{
		DBG_STR("Config Socket DisConnect Complete!\r\n");
		g_state_data.ConfigSocketConnected = UNCONNT;
	}
	tcpRxIndex[pStreamCxt->index].isUsed = FALSE;
}
A_UINT8 tcp_service_task_create(A_UINT32 mode,A_UINT32 IpAddress, IP6_ADDR_T Ip6Address, A_UINT32 Port)
{
	A_UINT32 m;
	A_UINT8 index = 0;
	A_UINT8 ret = 0;
	/* TCP Calc Handle */
	if(Port == g_sys_config.config_port)
		index = TCP_CFGPARAM_IDX;
	else if(Port == g_sys_config.data_port)
		index = TCP_DATPARAM_IDX;
	else
	{
		DBG_STR("tcp_service_task_create: Invalidate Port...\r\n");
		return FALSE;
	}

	DBG_TRACE("tcp_service_task_create:  Port(%d) , index(%d)...\r\n",Port,index);

	if(tcpRxIndex[index].isUsed)
	{
		DBG_STR("tcp_service_task_create: Port Is Used...\r\n");
		return FALSE;
	}
	if(index == TCP_DATPARAM_IDX)
		g_state_data.DataSocketConnected = CONNTING;
	if(index == TCP_CFGPARAM_IDX)
		g_state_data.ConfigSocketConnected = CONNTING;
	/* DataBase Initial */
	swat_database_initial(&cxtTcpRxPara[index]);
	/* Update DataBase */
	m = (index == TCP_CFGPARAM_IDX) ? sizeof(tcpCtrl_recv_buf) : sizeof(tcpData_recv_buf);
	swat_database_set(&cxtTcpRxPara[index], 	//pCxtPara
						IpAddress,//INADDR_ANY, 			//ipAddress
						&Ip6Address, 					//ip6Address
						INADDR_ANY, 			//mcAddress
						NULL,					//localIp6
						NULL, 					//mcastIp6
						Port,					//port
						0, 						//protocol
						0, 						//ssl_inst_index
						m,						//pktSize
						0,						//mode
						0, 						//seconds
						0, 						//numOfPkts
						TEST_DIR_RX, 			//direction
						0,						//local_ipAddress
						0, 						//ip_hdr_inc
						0, 						//rawmode
						0, 						//delay
						0, 						//iperf_mode
						0, 						//interval
						0);						//udpRate
	cxtTcpRxPara[index].param.pktBuff = (index == TCP_CFGPARAM_IDX) ? tcpCtrl_recv_buf : tcpData_recv_buf;
	cxtTcpRxPara[index].index = index;
	cxtTcpRxPara[index].param.devMode = mode;
	cxtTcpRxPara[index].param.procStat = PINIT_STAT;
	tcpRxIndex[index].isUsed = TRUE;
	ret = swat_task_create(tcp_transfer_task, index, 2048, 50);
	if (0 == ret)
	{
		DBG_TRACE("TCP Port %d Success\r\n", index);
	}else
	{
		tcpRxIndex[index].isUsed = FALSE;
		if(index == TCP_DATPARAM_IDX)
			g_state_data.DataSocketConnected = UNCONNT;
		if(index == TCP_CFGPARAM_IDX)
			g_state_data.ConfigSocketConnected = UNCONNT;
	}
	return tcpRxIndex[index].isUsed;
}

void tcp_open(A_UINT32 mode,A_UINT32 IpAddress,A_UINT32 Port)
{
	IP6_ADDR_T ipv6Address;
	A_MEMSET(&ipv6Address,0,sizeof(IP6_ADDR_T));
	DBG_STR("tcp_open...\r\n");
	if(v6_enabled)
	{
		Inet6Pton(_inet_ntoa(IpAddress), &ipv6Address);
	}
	if(tcp_service_task_create(mode,IpAddress,ipv6Address,Port)==TRUE)
	{
		DBG_STR("tcp_task Build Sucess...\r\n");
	}else
	{
		DBG_STR("tcp_task build Fail...\r\n");
	}
}



