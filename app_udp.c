/*
 * app_udp.c
 *
 *  Created on: Nov 30, 2017
 *      Author: root
 */

#include "qcom_common.h"
#include "app_common.h"
#include "qcom/socket_api.h"
#include "select_api.h"
#include "qcom_event.h"
#include "qcom_multicast_filter.h"
#include "paramconfig.h"
#include "app_main.h"
#include "devDbg.h"
#include "app_netmsg.h"

#pragma pack(4)
static A_UINT8 udpCtrl_send_buf[2048],udpCtrl_recv_buf[2048];
#pragma   pack()
static A_UINT32 savedPowerMode0 = REC_POWER;
static A_UINT32 savedPowerMode1 = REC_POWER;
static A_INT32 savedPowerModeCount = 0;
int v6_enabled=0;
void swat_bench_pwrmode_maxperf()
{
   savedPowerModeCount++;
   if (savedPowerModeCount == 1)
   {
    	qcom_power_get_mode(0, &savedPowerMode0);
    	if(savedPowerMode0!=MAX_PERF_POWER)
        	qcom_power_set_mode(0, MAX_PERF_POWER);
       if (gNumOfWlanDevices>1)
       {
        	qcom_power_get_mode(1, &savedPowerMode1);
        	if(savedPowerMode1!=MAX_PERF_POWER)
            	qcom_power_set_mode(1, MAX_PERF_POWER);
       }
   }
}

void swat_bench_restore_pwrmode()
{
   if (savedPowerModeCount<=0)
      return;
   savedPowerModeCount--;
   if (savedPowerModeCount == 0)
   {
    	if(savedPowerMode0!=MAX_PERF_POWER)
        	qcom_power_set_mode(0, REC_POWER);
      if (gNumOfWlanDevices>1)
      {
        	if(savedPowerMode1!=MAX_PERF_POWER)
            	qcom_power_set_mode(1, REC_POWER);
       }
   }
}
extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);

typedef struct _HCTRL_UDP_PARAM
{
	STREAM_CXT_t *pStreamCxt;
	struct sockaddr *fromAddr;
	NET_MSG *lpMsg;
}HCTRL_UDP_PARAM;


static s32 udp_send_dats(u8 *lpDat,u16 dLen,void *p)
{
	HCTRL_UDP_PARAM *lp = (HCTRL_UDP_PARAM*)p;
	MSG_HD *lpMsgHd = (MSG_HD*)udpCtrl_send_buf;
	A_INT32 sendBytes;
	//struct sockaddr_in *lpIP;

	if(!(lp && lpDat && dLen))
		return -1;

	*lpMsgHd = lp->lpMsg->hdPkt;

	if(dLen > (sizeof(udpCtrl_send_buf) - NET_MSGHD_SIZE))
	{
		DBG_TRACE("udp_send_dats : Send Datas too more(%d)...\r\n",dLen);
		return dLen;
	}

	lpMsgHd->len = dLen;
	memcpy(&udpCtrl_send_buf[NET_MSGHD_SIZE],lpDat,dLen);

	sendBytes = swat_sendto(lp->pStreamCxt->socketLocal,
			(char*)lpMsgHd,dLen + NET_MSGHD_SIZE, 0,
			lp->fromAddr,
			lp->pStreamCxt->param.is_v6_enabled ? sizeof (struct sockaddr_in6) : sizeof (struct sockaddr_in));

	if (sendBytes < 0)
	{
		SWAT_PTF("UDP TX ERROR.\n");
	}

	return sendBytes - NET_MSGHD_SIZE;
}
static void udp_data_io(STREAM_CXT_t * pStreamCxt)
{
	struct sockaddr_in fromAddr;
	struct sockaddr_in6 from6Addr;
	A_INT32 recvBytes = 0;
	q_fd_set sockSet, master;
	A_INT32 fromSize = 0;
	struct timeval tmo;
	A_INT32 fdAct = 0;

	DBG_STR("swat_udp_rx_data...\r\n");

	SWAT_PTR_NULL_CHK(pStreamCxt);

	/* Initial Bench Value */
	swat_bench_quit_init();
	pStreamCxt->param.pktBuff = udpCtrl_recv_buf;
	pStreamCxt->param.pktSize = sizeof(udpCtrl_recv_buf);
	/* Prepare IP address & port */
	if (pStreamCxt->param.is_v6_enabled)
	{
		memset(&from6Addr, 0, sizeof(struct sockaddr_in6));
		fromSize = sizeof(struct sockaddr_in6);
	} else
	{
		memset(&fromAddr, 0, sizeof(struct sockaddr_in));
		fromSize = sizeof(struct sockaddr_in);
	}

	/* Init fd_set */
	swat_fd_zero(&master);
	swat_fd_set(pStreamCxt->socketLocal, &master);
	pStreamCxt->pfd_set = (void *) &master;

	tmo.tv_sec = 0;
	tmo.tv_usec = 10;

	//-----------------------------------------------
	NET_MSG mNetMsg;
	HCTRL_UDP_PARAM param;

	param.pStreamCxt = pStreamCxt;
	param.lpMsg = &mNetMsg;
	param.fromAddr = (pStreamCxt->param.is_v6_enabled) ?
						(struct sockaddr *) &from6Addr :
						(struct sockaddr *)&fromAddr;

	HCTRL_PORT hPort;

	initCtrlPort(&hPort,NULL,NULL,udp_send_dats,(void*)&param);
	HCTRL_PHANDLE hCtrlHandle = regCtrlPort(&hPort);

	netMsg_init(&mNetMsg);
	mNetMsg.ctrlHandle = hCtrlHandle;

	while (1)
	{
		if (swat_bench_quit())// || (pStreamCxt->socketLocal < 0))
			break;

		if(mNetMsg.datLen)
		{
			if( netMsg_proc(&mNetMsg) < 0)
			{
				DBG_STR("udp_data_io : netMsg_proc Fail \r\n");
				mNetMsg.datLen = 0;
			}
		}

		/* Copy the master list everytime before calling select() because select
		 * modifies sockSet */
		sockSet = master; /* Wait for Input */
		fdAct = swat_select(pStreamCxt->socketLocal + 1, &sockSet, NULL, NULL, &tmo); //k_select()
		if (fdAct < 0)
		{
			SWAT_PTF("select err");
			break;
		}else if (fdAct == 0)
		{
			tx_thread_sleep(1);
			//qcom_thread_msleep(100);
			continue;
		}
		if (swat_fd_isset(pStreamCxt->socketLocal, &sockSet) && (pStreamCxt->socketLocal > 0))
		{
			recvBytes = swat_recvfrom(pStreamCxt->socketLocal,
				(char*) pStreamCxt->param.pktBuff,
				pStreamCxt->param.pktSize,
				0,
				(pStreamCxt->param.is_v6_enabled) ? (struct sockaddr *) &from6Addr : (struct sockaddr *) &fromAddr,
				&fromSize);
			if (recvBytes < 0)
			{
				SWAT_PTF("UDP Socket receive is error\r\n");
			} else if (recvBytes < 0)
			{
				SWAT_PTF("Socket close by Clicent!\r\n");
			} else
			{
				//DBG_TRACE("recvBytes = %d ... stat = %d,%d\r\n",recvBytes,getCtrlTokenStat(hCtrlHandle),fromSize);
				netMsg_reset(&mNetMsg);

				mNetMsg.datLen = recvBytes;
				mNetMsg.lpDats = pStreamCxt->param.pktBuff;

				mNetMsg.timer = time_ms();
			}
			swat_fd_clr(pStreamCxt->socketLocal, &sockSet);
		}
	}

	swat_socket_close(pStreamCxt);
	unRegCtrlPort(hCtrlHandle);
	/* Free Buffer */
}

static void udp_transfer_task(A_UINT32 index)
{
	A_INT32 ret = 0;
	A_INT32 i = -1;
	STREAM_CXT_t * pStreamCxt = &cxtUdpRxPara[index];
	struct sockaddr_in remoteAddr;
	struct sockaddr_in6 remoteAddr6;
	struct ip_mreq
	{
		A_UINT32 imr_multiaddr; /* IP multicast address of group */
		A_UINT32 imr_interface; /* local IP address of interface */
	} group;
	struct ipv6_mreq
	{
		IP6_ADDR_T ipv6mr_multiaddr; /* IPv6 multicast addr */
		IP6_ADDR_T ipv6mr_interface; /* IPv6 interface address */
	} group6;

	while (g_state_data.WifiConnected !=CONNTED)
	{
		if(swat_bench_quit())
			goto QUIT;

		qcom_thread_msleep(10);
	}

	SWAT_PTR_NULL_CHK(pStreamCxt);
	swat_bench_pwrmode_maxperf();
	while (++i < CALC_STREAM_NUMBER)
	{
		if ((pStreamCxt->param.port == cxtUdpRxPara[i].param.port)&& (i != pStreamCxt->index))
		{
			SWAT_PTF("Bind Failed!\n");
			swat_socket_close(pStreamCxt);
			goto QUIT;
		}
	}
	if (pStreamCxt->param.is_v6_enabled)
	{
		pStreamCxt->socketLocal = swat_socket(PF_INET6, SOCK_DGRAM, 0);
		SWAT_PTF("SOCKET %d\n", pStreamCxt->socketLocal);
		if (pStreamCxt->socketLocal < 0)
			goto QUIT;

		/* Bind Socket */
		swat_mem_set(&remoteAddr6, 0, sizeof(struct sockaddr_in6));
		A_MEMCPY(&remoteAddr6.sin6_addr, pStreamCxt->param.ip6Address.addr,sizeof(IP6_ADDR_T));
		remoteAddr6.sin6_port = htons(pStreamCxt->param.port);
		remoteAddr6.sin6_family = AF_INET6;
		if ((ret = swat_bind(pStreamCxt->socketLocal, (struct sockaddr *) &remoteAddr6,sizeof (struct sockaddr_in6)))< 0)
		{
			/* Close Socket */
			SWAT_PTF("Bind Failed\n");
			swat_socket_close(pStreamCxt);
			goto QUIT;
		}

		if (pStreamCxt->param.mcast_enabled)
		{
			memcpy(&group6.ipv6mr_multiaddr, pStreamCxt->param.mcastIp6.addr,
					sizeof(IP6_ADDR_T));
			memcpy(&group6.ipv6mr_interface, pStreamCxt->param.localIp6.addr,
					sizeof(IP6_ADDR_T));
			if ((ret =swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, IPV6_JOIN_GROUP,(void*)(&group6),sizeof(struct ipv6_mreq)))< 0)
			{
				SWAT_PTF("SetsockOPT error : unable to add to multicast group\r\n");
				swat_socket_close(pStreamCxt);
				goto QUIT;
			}
		}
	} else
	{
		if ((pStreamCxt->socketLocal = swat_socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		{
			SWAT_PTF("Open UDP socket error...\r\n");
			goto QUIT;
		}

		/* Connect Socket */
		swat_mem_set(&remoteAddr, 0, sizeof(struct sockaddr_in));
		remoteAddr.sin_addr.s_addr = htonl(pStreamCxt->param.ipAddress);
		remoteAddr.sin_port = htons(pStreamCxt->param.port);
		remoteAddr.sin_family = AF_INET;

		if ((ret = swat_bind(pStreamCxt->socketLocal, (struct sockaddr *) &remoteAddr,sizeof (struct sockaddr_in))) < 0)
		{
			/* Close Socket */
			SWAT_PTF("Failed to bind socket %d.\n", pStreamCxt->socketLocal);
			swat_socket_close(pStreamCxt);
			goto QUIT;
		}
		if (pStreamCxt->param.mcast_enabled)
		{
			group.imr_multiaddr = HTONL(pStreamCxt->param.mcAddress);
			group.imr_interface = HTONL(pStreamCxt->param.local_ipAddress);
			if (group.imr_multiaddr != 0)
			{
				if ((ret = swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, IP_ADD_MEMBERSHIP, (void *) &group,sizeof (group)))< 0)
				{
					/* Close Socket */
					SWAT_PTF("Failed to set socket option %d.\n",pStreamCxt->socketLocal);
					swat_socket_close(pStreamCxt);
					goto QUIT;
				}
			}
		}
	}

	g_state_data.UdpConnected = CONNTED;

	DBG_TRACE("UDP Connected : Local port %d\r\n", pStreamCxt->param.port);
	DBG_STR("Waiting Data...\r\n");

	/* Packet Handle */
	udp_data_io(pStreamCxt);//udp_data_handle(pStreamCxt);

	DBG_STR("udp_transfer_task exit.\r\n");

QUIT:
	g_state_data.UdpConnected = UNCONNT;

	swat_bench_restore_pwrmode();
	/* Free Index */
	swat_cxt_index_free(&udpRxIndex[pStreamCxt->index]);
	swat_bench_restore_pwrmode();
	/* Thread Delete */
	swat_task_delete();
}
A_UINT8 udp_open(A_UINT8 mode,A_UINT32 port,A_UINT32 localIpAddress)
{
	A_UINT8 index = 0;
	A_UINT8 ret = 0;

	if(mode != SERVICE_MODE)
		return 0;

	DBG_STR("udp_open...\r\n");

    /*Calc Handle */
	for (index = 0; index < CALC_STREAM_NUMBER; index++)
	{
		ret = swat_cxt_index_find(&udpRxIndex[index]);

		if (CALC_STREAM_NUMBER_INVALID != ret)
		{
			swat_cxt_index_configure(&udpRxIndex[index]);
            break;
		}else
        {
			if (cxtUdpRxPara[index].param.port == port)
            {
				SWAT_PTF("This port is in use, use another Port.\n");
            	return 0;
            }
        }
	}

	if (CALC_STREAM_NUMBER_INVALID == ret)
		return 0;

	/* DataBase Initial */
	swat_database_initial(&cxtUdpRxPara[index]);

	/* Update DataBase */
	swat_database_set(&cxtUdpRxPara[index],
						INADDR_ANY,
						NULL,
						0,//mcastIpAddress,
						NULL,//lIp6,
						NULL,//mIp6,
						port,
						TEST_PROT_UDP,//protocol,
                        0,
                        1024,
                        0,
                        0,
                        0,
                        TEST_DIR_RX,
                        localIpAddress,
                        0,
                        0,
                        0,
                        0,//iperf_mode,
                        0,//interval,
                        0);

	/* Server */
	cxtUdpRxPara[index].index = index;

	/* Server */
	g_state_data.UdpConnected = CONNTING;
	ret = swat_task_create(udp_transfer_task, index, 2048+512, 50);
	if (0 != ret)
	{
		swat_cxt_index_free(&udpRxIndex[index]);
		g_state_data.UdpConnected = UNCONNT;
	} else
    {
		SWAT_PTF("UDP RX session %d Success\n", index);
    }
	return ret;
}

















