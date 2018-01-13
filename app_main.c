/*
 * Copyright (c) 2013 Qualcomm Atheros, Inc..
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#include "qcom_common.h"
#include "qcom_uart.h"
#include "app_common.h"
#include "paramconfig.h"
#include "app_main.h"
#include "devDbg.h"
#include "HCtrlProto.h"
#include "app_def_sys_err.h"
#include "app_tcp.h"
#include "app_udp.h"
#include "app_gpio.h"
#include "app_dev.h"
#include "app_dns.h"
DNS_GET_PARAM  Dns;
struct sys_config g_sys_config;
SYS_STAT_INFO g_state_data;
void initCtrlProto(void);
void app_dev_led_init(void);
extern A_UINT8 bridge_mode;
static void Scan_Info_Display(A_UINT16 count,QCOM_BSS_SCAN_INFO* info)
{
	int i;
	for (i = 0; i < count; i++)
	{
		DBG_TRACE("ssid =%s\n", (char*)&info[i].ssid);
		DBG_TRACE("bssid = %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
					info[i].bssid[0],info[i].bssid[1], info[i].bssid[2],
					info[i].bssid[3],info[i].bssid[4], info[i].bssid[5]);
		DBG_TRACE("channel = %d\n", info[i].channel);
		DBG_TRACE("indicator = %d\n", info[i].rssi);
		if (info[i].security_enabled)
		{
			DBG_STR("security = \n");
			if (info[i].rsn_auth || info[i].rsn_cipher)
			{
				DBG_STR("RSN/WPA2= ");
				if (info[i].rsn_auth)
				{
					if (info[i].rsn_auth & SECURITY_AUTH_1X)
						DBG_STR("{802.1X }");
					if (info[i].rsn_auth & SECURITY_AUTH_PSK)
						DBG_STR("{PSK }");
				}
				if (info[i].rsn_cipher)
				{
					if (info[i].rsn_cipher & ATH_CIPHER_TYPE_WEP)
						DBG_STR("{WEP }");
					if (info[i].rsn_cipher & ATH_CIPHER_TYPE_TKIP)
						DBG_STR("{TKIP }");
					if (info[i].rsn_cipher & ATH_CIPHER_TYPE_CCMP)
						DBG_STR("{AES }");
				}
				DBG_STR("\n");
			}
			if (info[i].wpa_auth || info[i].wpa_cipher)
			{
				DBG_STR("WPA= ");
				if (info[i].wpa_auth)
				{
					if (info[i].wpa_auth & SECURITY_AUTH_1X)
						DBG_STR("{802.1X }");
					if (info[i].wpa_auth & SECURITY_AUTH_PSK)
						DBG_STR("{PSK }");
				}
				if (info[i].wpa_cipher)
				{
					if (info[i].wpa_cipher & ATH_CIPHER_TYPE_WEP)
						DBG_STR("{WEP }");
					if (info[i].wpa_cipher & ATH_CIPHER_TYPE_TKIP)
						DBG_STR("{TKIP }");
					if (info[i].wpa_cipher & ATH_CIPHER_TYPE_CCMP)
						DBG_STR("{AES }");
				}
				DBG_STR("\n");
			}
			if (info[i].rsn_cipher == 0 && info[i].wpa_cipher == 0)
				DBG_STR("WEP \n");
		}else
			DBG_STR("security = NONE! \n");
		if (i != (count - 1))
			DBG_STR("\n");
	}
}
static A_INT32 wifi_scanf(A_CHAR * softApssid)
{
	A_UCHAR wifi_state = 0;
	A_CHAR* pssid;
	A_UINT16 count;
	QCOM_BSS_SCAN_INFO* pOut;
	A_CHAR saved_ssid[WMI_MAX_SSID_LEN + 1];
	qcom_set_scan_timeout(5);
	if (qcom_get_state(currentDeviceId, &wifi_state) != SYS_OK)
		return SYS_ERR01;
	if (wifi_state == QCOM_WLAN_LINK_STATE_DISCONNECTED_STATE)
	{
		A_MEMSET(saved_ssid, 0, WMI_MAX_SSID_LEN + 1);
		if (qcom_get_ssid(currentDeviceId, saved_ssid) != SYS_OK)
			return SYS_ERR02;
		if (wifi_state != QCOM_WLAN_LINK_STATE_CONNECTING_STATE)
			pssid = softApssid;
		/*Set the SSID*/
		if (qcom_set_ssid(currentDeviceId, pssid) != SYS_OK)
			return SYS_ERR03;
		/*Start scan*/
		qcom_start_scan_params_t scanParams;
		scanParams.forceFgScan = 1;
		scanParams.scanType = WMI_LONG_SCAN;
		scanParams.numChannels = 0;
		scanParams.forceScanIntervalInMs = 0;
		scanParams.homeDwellTimeInMs = 1;
		if (qcom_set_scan(currentDeviceId, &scanParams) != SYS_OK)
			return SYS_ERR04;
		/*Get scan results*/
		if (qcom_get_scan(currentDeviceId, &pOut, &count) == SYS_OK)
		{
			if (count <= 0)
			{
				printf("Have no Found AP\r\n");
				g_state_data.WifiConnected = UNCONNT;
				return SYS_OK;
			}
			else
			{
				DBG_TRACE("Scanf (%d) Wifi\r\n", count);
				Scan_Info_Display(count, pOut);
			}
		}
		if (qcom_set_ssid(currentDeviceId, saved_ssid) != SYS_OK)
			return SYS_ERR05;
	}
	return SYS_OK;
}
static A_INT32 password_auth(A_CHAR * password)
{
	A_UINT32 str_len = 0, i = 0;
	A_CHAR psd_buf[65];
	str_len = strlen(password);
	if ((str_len < 8) || (str_len > 64))
	{
		DBG_STR("PSK passcode len should in 8~63 in ASCII and 8~64 in HEX \n");
		return SYS_ERR06;
	}
	else if (str_len == 64)
	{
		/*wpa2-psk passwd len is 64, the passwd must be NON ASCII char*/
		A_STRNCPY(psd_buf, password, 64);
		for (i = 0; i < 64; i++)
		{
			if ((psd_buf[i] < 48) || ((psd_buf[i] > 57) && (psd_buf[i] < 64))
					|| ((psd_buf[i] > 90) && (psd_buf[i] < 97))
					|| (psd_buf[i] > 123))
			{
				DBG_STR(
						"PSK passcode len is 64 and can't contain Non 16 HEX char.\n");
				return SYS_ERR07;
			}
		}
		qcom_sec_set_passphrase(currentDeviceId, password);
	}
	else if (qcom_sec_set_passphrase(currentDeviceId, password) != SYS_OK)
	{
		return SYS_ERR08;
	}
	DBG_TRACE("passwd:%s,currentDeviceId:%d,str_len:%ld\r\n", password,currentDeviceId, str_len);
	return SYS_OK;
}
static void wifi_conn_callback(A_UINT8 device_id, int value)
{
  QCOM_BSS_SCAN_INFO *bss_info = NULL;
  A_UINT8 bssid[6];
  if (value == 10)
  {
    DBG_STR("[CB]disconnected - INVALID_PROFILE \n");
    value = 0;
  }
  if (value)
  {
    DBG_TRACE("Connected to %s, ", gDeviceContextPtr[device_id]->ssid);
    bss_info = qcom_get_bss_entry_by_ssid(device_id, (char *)gDeviceContextPtr[device_id]->ssid);
    if (NULL == bss_info)
    {
      return;
    }
    A_MEMCPY(bssid, bss_info->bssid, 6);
    DBG_TRACE("bssid = %02x-%02x-%02x-%02x-%02x-%02x, ", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    DBG_TRACE("channel = %d, rssi = %d\n", bss_info->channel, bss_info->rssi);
    DBG_TRACE("Value:%d\r\n",value);
    qcom_sta_reconnect_stop(device_id);
  }
  else
  {
	DBG_TRACE("Disconnected from %s\n", gDeviceContextPtr[device_id]->ssid);
    bss_info = qcom_get_bss_entry_by_ssid(device_id, (char *)gDeviceContextPtr[device_id]->ssid);
    if (NULL == bss_info)
    {
    	return;
    }
    DBG_TRACE("channel = %d, rssi = %d\n", bss_info->channel, bss_info->rssi);
    qcom_sta_reconnect_start(device_id,5);
  }
  g_state_data.WifiConnected =CONNTING;
}
static void app_set_dhcp_pool(A_UINT8 device_id, A_CHAR * pStartaddr, A_CHAR * pEndaddr, A_UINT32 leasetime)
{
   A_UINT32 startaddr, endaddr;

   startaddr = _inet_addr(pStartaddr);
   endaddr = _inet_addr(pEndaddr);
   qcom_dhcps_set_pool(device_id, startaddr, endaddr, leasetime);
}
static A_BOOL app_check_netmask(A_UINT32 submask)
{
	A_UINT32 i;
	A_UINT32 flone = 31;
	A_UINT32 fhzero =0;
	A_BOOL valid;

	for(i=0;i<32;i++)
	{
		if((submask>>i)&0x1)
		{
			flone = i;
			break;
		}
	}
	for(i=0;i<32;i++)
	{
		if(!((submask<<i)&0x80000000))
		{
			fhzero = 31-i;
			break;
		}
	}
	valid = (flone>=fhzero)&&(fhzero<31);
	return valid;
}
static void app_set_ipstatic(A_UINT8 device_id, A_CHAR * pIpAddr, A_CHAR * pNetMask, A_CHAR * pGateway)
{
	A_UINT32 address;
	A_UINT32 submask;
	A_UINT32 gateway;
	A_UINT32 valid_addr[4];
	A_UINT32 rc;
	submask = g_sys_config.host_submask;
	if(!app_check_netmask(submask))
	 {
		  SWAT_PTF("Invalid submask.\n");
		  submask=_inet_addr("255.255.255.0");
	  }
	 /*C tpye address ip address region should be in 1~254*/
	 rc = A_SSCANF(pIpAddr, "%3d.%3d.%3d.%3d", valid_addr, valid_addr + 1, valid_addr + 2, valid_addr+ 3);
	 if(rc < 0)
	  {
		 address = _inet_addr("192.168.1.10");
		 gateway = _inet_addr("192.168.1.10");
	  }else if((valid_addr[3] >= 255)||(valid_addr[3] == 0))
	  {
		  address = _inet_addr("192.168.1.10");
		  gateway = _inet_addr("192.168.1.10");
	  }else
	  {
		  address = _inet_addr(pIpAddr);
		  gateway = _inet_addr(pGateway);
	  }
	 qcom_ipconfig(device_id, IP_CONFIG_SET, &address, &submask, &gateway);
}
static A_INT32 wifi_connect(A_CHAR * wifiname, A_CHAR * password, A_UINT32 devMode)
{
	char Static_IP[17] = { 0 }, Static_Submask[17] = { 0 },Static_Gatway[17] = { 0 };
	if ((devMode != QCOM_WLAN_DEV_MODE_AP)&& (devMode != QCOM_WLAN_DEV_MODE_STATION))
	{
		return SYS_ERR09;
	}
	if (devMode == QCOM_WLAN_DEV_MODE_AP)
	{
		qcom_power_set_mode(currentDeviceId, MAX_PERF_POWER);
	} else if (devMode == QCOM_WLAN_DEV_MODE_STATION)
	{
		qcom_power_set_mode(currentDeviceId, REC_POWER);
		qcom_set_hostname(0, "DascomDevice");
	}
	if (password_auth(password) != SYS_OK)
	{
		return SYS_ERR10;
	}
	if (qcom_set_connect_callback(0, wifi_conn_callback)!= SYS_OK)
	{
		return SYS_ERR11;
	}
	if (qcom_set_ssid(currentDeviceId, wifiname) != SYS_OK)
	{
		return SYS_ERR12;
	}
	if ((qcom_commit(currentDeviceId) != SYS_OK))
	{
		return SYS_ERR13;
	}
	if ((currentDeviceId == 0)&&(devMode == QCOM_WLAN_DEV_MODE_AP)&&(bridge_mode == 0)) // AP mode
	{
		memcpy(Static_IP,_inet_ntoa(g_sys_config.host_ip),sizeof(Static_IP));
		memcpy(Static_Gatway,_inet_ntoa(g_sys_config.host_gateway),sizeof(Static_Gatway));
		memcpy(Static_Submask,_inet_ntoa(g_sys_config.host_submask),sizeof(Static_Submask));
		DBG_TRACE("static ip:%s\r\n", Static_IP);
		DBG_TRACE( "static_submask:%s\r\n",Static_Submask);
		DBG_TRACE("Static_Gatway:%s\r\n",Static_Gatway);
		app_set_ipstatic(currentDeviceId, (A_CHAR*) Static_IP,(A_CHAR*) Static_Submask, (A_CHAR*) Static_Gatway);
		app_set_dhcp_pool(currentDeviceId, "192.168.1.100","192.168.1.200", 0xFFFFFFFF);
	}
	return SYS_OK;
}
static A_INT32 Station_Init(void)
{
	A_UINT32 param = 0;
	DBG_STR("Station_Init\r\n");
	if(qcom_set_hostname(currentDeviceId,g_sys_config.ap_ssid) != SYS_OK)
	{
		return SYS_ERR14;
	}
	swat_wmiconfig_wpa_set(0, "2", "CCMP", "CCMP");
	if(wifi_scanf(g_sys_config.conn_ap_ssid) != SYS_OK)
	{
		return SYS_ERR15;
	}
	if(g_state_data.WifiConnected == UNCONNT)
	{
		return SYS_OK;
	}
	if(wifi_connect(g_sys_config.conn_ap_ssid, g_sys_config.conn_ap_password,QCOM_WLAN_DEV_MODE_STATION) != SYS_OK)
	{
		DBG_TRACE("Station_Init : wifi connect(%s) fail\r\n",g_sys_config.conn_ap_ssid);
		return SYS_ERR16;
	}else
	{
		qcom_ipconfig(currentDeviceId, IP_CONFIG_DHCP, &param, &param, &param);
	}
	if(qcom_get_state(currentDeviceId, (A_UINT8* )&g_state_data.state)!= SYS_OK)
	{
		return SYS_ERR17;
	}
	return SYS_OK;
}
static A_INT32 SoftAp_Init(void)
{
	DBG_STR("SoftAp_Init\r\n");
	if (SYS_OK != qcom_ap_set_max_station_number(currentDeviceId, 4))
	{
		DBG_STR("SoftAp_Init : set max station fail\r\n");
		return SYS_ERR18;
	}
	if (wifi_connect(g_sys_config.ap_ssid, g_sys_config.ap_password,QCOM_WLAN_DEV_MODE_AP) != SYS_OK)
	{
		DBG_TRACE("SoftAp_Init : wifi connect(%s) fail\r\n",g_sys_config.ap_ssid);
		return SYS_ERR19;
	}
	return SYS_OK;
}
static A_INT32 set_wifi_channle(A_UINT32 channel)
{
	int status;
	status =qcom_set_channel(currentDeviceId, channel);
	if(status!=SYS_OK)
	{
		DBG_STR("Channel is invalid.\r\n");
		return SYS_ERR20;
	}else
	{
		DBG_TRACE("Channel is %ld\r\n",channel);
	}
	return SYS_OK;
}
static A_INT32 param_InitLib(void)
{
	A_INT32 status=SYS_OK;
	if(qcom_op_set_mode(currentDeviceId,g_sys_config.wifi_mode)!=SYS_OK)
	{
		return SYS_ERR21;
	}
	if (g_sys_config.wifi_mode == QCOM_WLAN_DEV_MODE_AP)
	{
		DBG_STR("param_InitLib : WIFI Enter AP Mode\r\n");
		status=SoftAp_Init();
		if(status!=SYS_OK)
			return status;
	}else if(g_sys_config.wifi_mode == QCOM_WLAN_DEV_MODE_STATION)
	{
		DBG_STR("param_InitLib : WIFI Enter Station Mode\r\n");
		if( A_ERROR == qcom_sta_reconnect_start(currentDeviceId,g_sys_config.recnnt_interval))
		{
			DBG_STR("param_InitLib : qcom_sta_reconnect_start fail\r\n");
			return SYS_ERR22;
		}
	}else
	{
		DBG_STR("param_InitLib : Invalid WIFI Mode\r\n");
	}
	return set_wifi_channle(g_sys_config.channel);
}
static A_STATUS sysy_init(void)
{
	memset(&g_state_data,UNCONNT,sizeof(g_state_data));
	dev_sys_gpio_init();
	app_dev_led_init();
	loadSysCfg(&g_sys_config);
	if(uart_port_open()!=A_OK)
		return SYS_ERR29;
	else if(spi_port_open()!=A_OK)
		return SYS_ERR30;
	else
		initCtrlProto();
	return A_OK;
}
static A_INT32 wifi_station_stat(void)
{
	A_INT32 status=SYS_OK;
	A_UINT32 m,n = 0;
	A_UINT8 connStat;
	IP6_ADDR_T v6;
	memset(&v6,0,sizeof(v6));
	static A_UINT8 oldStat = 0xff;
	if (qcom_get_state(currentDeviceId, (A_UINT8* )&connStat)!= SYS_OK)
		return SYS_ERR23;

	switch(connStat)
	{
		case QCOM_WLAN_LINK_STATE_DISCONNECTED_STATE:
			DBG_STR_ONCE(oldStat, connStat,"QCOM_WLAN_LINK_STATE_DISCONNECTED_STATE...\r\n");
			if (g_state_data.WifiConnected == UNCONNT)
			{
				g_state_data.WifiConnected = CONNTING;
				status = Station_Init();
				if (status != SYS_OK)
					return status;
			}else if (g_state_data.WifiConnected ==CONNTED)
			{
				DBG_STR("qcom_sta_reconnect_start...\r\n");
				g_state_data.WifiConnected = CONNTING;
			}
			break;
		case QCOM_WLAN_LINK_STATE_STARTING_STATE:
			DBG_STR_ONCE(oldStat, connStat,"QCOM_WLAN_LINK_STATE_STARTING_STATE...\r\n");
			break;
		case QCOM_WLAN_LINK_STATE_CONNECTING_STATE:
			DBG_STR_ONCE(oldStat, connStat,"QCOM_WLAN_LINK_STATE_CONNECTING_STATE...\r\n");
			break;
		case QCOM_WLAN_LINK_STATE_AUTHENTICATING_STATE:
			DBG_STR_ONCE(oldStat, connStat,"QCOM_WLAN_LINK_STATE_AUTHENTICATING_STATE...\r\n");
			break;
		case QCOM_WLAN_LINK_STATE_CONNECTED_STATE:
			DBG_STR_ONCE(oldStat, connStat,"QCOM_WLAN_LINK_STATE_CONNECTED_STATE...\r\n");
			if (g_state_data.WifiConnected != CONNTED)
			{
				qcom_ipconfig(currentDeviceId, IP_CONFIG_QUERY, &n, &m, &m);
				if (n)
				{
					DBG_TRACE("ipv4 = %d.%d.%d.%d\r\n", (A_UINT8 )(n >> 24),(A_UINT8 )(n >> 16), (A_UINT8 )(n >> 8), (A_UINT8 )(n));
					app_dns_para_set(&Dns, _inet_ntoa(g_sys_config.dns_ip), 10,_inet_ntoa(g_sys_config.resv_dns_ip), g_sys_config.URL,AF_INET, NULL, 0, v6, Enable, Enable, RESOLVEHOSTNAME,NULL);
					if (app_dns_proc(&Dns) == A_OK)
					{
						g_sys_config.server_ip = Dns.ipv4;
						SWAT_PTF("g_sys_config.server_ip:%s\r\n",_inet_ntoa(g_sys_config.server_ip));
					}
					g_state_data.WifiConnected =CONNTED;
				}
			}
			if (!g_state_data.UdpConnected)
				udp_open(SERVICE_MODE, g_sys_config.udp_port, INADDR_ANY);
			if (!g_state_data.ConfigSocketConnected)
				tcp_open(CLIENT_MODE, g_sys_config.server_ip,g_sys_config.config_port);
			break;
		case QCOM_WLAN_LINK_STATE_DISCONNECTING_STATE:
			DBG_STR_ONCE(oldStat, connStat,"QCOM_WLAN_LINK_STATE_DISCONNECTING_STATE...\r\n");
			break;
		case QCOM_WLAN_LINK_STATE_WEP_KEY_NOT_MATCH:
			DBG_STR_ONCE(oldStat, connStat,"QCOM_WLAN_LINK_STATE_WEP_KEY_NOT_MATCH...\r\n");
			break;
		default:
			DBG_STR_ONCE(oldStat, connStat,"udp_station_proc: unknown state...\r\n");
			break;
	}
	return SYS_OK;
}
static A_INT32 wifi_AP_stat(void)
{
	switch(g_state_data.sys_mode)
	{
	case OTA_MODE:
		break;
	default:
		if(!g_state_data.UdpConnected)
				udp_open(SERVICE_MODE, g_sys_config.udp_port,INADDR_ANY);
			if(g_state_data.ConfigSocketConnected == UNCONNT)
				tcp_open(SERVICE_MODE,INADDR_ANY,g_sys_config.config_port);
			if(g_state_data.DataSocketConnected == UNCONNT)
				tcp_open(SERVICE_MODE,INADDR_ANY,g_sys_config.data_port);
		break;
	}
	return SYS_OK;
}

static void powerup_func()
{
	A_UINT32 ButtonPressTime=0;

	while(!qcom_gpio_pin_get(DEV_HID_STAT_INFO_PIN))
	{
		if(!ButtonPressTime)
		{
			g_sys_config.wifi_mode = QCOM_WLAN_DEV_MODE_AP;
		}

		if(ButtonPressTime >= 100)//5*1000)
		{
			break;
		}

		ButtonPressTime++;
		qcom_thread_msleep(50);
	}

	if(ButtonPressTime >= 100)//5*1000)
	{
		SWAT_PTF("Ota Mode in to\r\n");
		g_state_data.sys_mode=OTA_MODE;
	}else if(ButtonPressTime)
	{
		SWAT_PTF("Config Mode in to\r\n");
		g_state_data.sys_mode=CONFIG_MODE;
	}
}
void main_task(void)
{
	A_INT32 status = SYS_OK;
	A_UINT32 pMode;

	if (sysy_init() == A_OK)
	{
		OPEN_DEBUG();
		DBG_TRACE("main_task....Ver(%02x.%02x)\r\n", (A_UINT8)(FW_VER >> 8),(A_UINT8)FW_VER);
		powerup_func();
		if (param_InitLib() == SYS_OK)
		{
			if (qcom_op_get_mode(currentDeviceId,&pMode) == A_OK)
			{
				while (1)
				{
					if(pMode == QCOM_WLAN_DEV_MODE_STATION)
						status = wifi_station_stat();
					else if(pMode == QCOM_WLAN_DEV_MODE_AP)
						status = wifi_AP_stat();
					if(status != SYS_OK)
						break;
					if(!g_state_data.sys_mode)
						app_spi_tx_proc();
					tx_thread_sleep(1);
				}
			}
		}
	}
	qcom_sys_reset();
}

void getSysStat(DSEMUL_UW_GENSTAT *sysStat)
{
	A_UINT32 m;

	sysStat->size = sizeof(DSEMUL_UW_GENSTAT);
	sysStat->itemType = DEVSTAT_TYPE_GEN;

	sysStat->sysMode = g_state_data.sys_mode;

	qcom_op_get_mode(currentDeviceId,&m);
	sysStat->wifiMode = (A_UINT8)m;

	sysStat->devStat = g_state_data.UsbDeviceState;

	sysStat->errCode[0] = 0;
	sysStat->errCode[1] = 1;

	sysStat->portStat[UDP_PORT_IDX] = g_state_data.UdpConnected;
	sysStat->portStat[CTRL_PORT_IDX] =	g_state_data.ConfigSocketConnected ;
	sysStat->portStat[DATA_PORT_IDX] = g_state_data.DataSocketConnected;
}
void getFlwStat(DSEMUL_UW_FLWSTAT *fStat)
{
	A_UINT32 m;

	fStat->size = sizeof(DSEMUL_UW_FLWSTAT);
	fStat->itemType = DEVSTAT_TYPE_FLOW;

	m = rngBufSpace(&gwrxBuf);
	fStat->recvSpace[0] = (A_UINT8)(m >> 24);
	fStat->recvSpace[1] = (A_UINT8)(m >> 16);
	fStat->recvSpace[2] = (A_UINT8)(m >> 8);
	fStat->recvSpace[3] = (A_UINT8)m;

	m = (gwrxBuf.len - m) * 1000 / gwrxBuf.len;
	fStat->recvRate[0] = (A_UINT8)(m >> 8);
	fStat->recvRate[1] = (A_UINT8)m;
}
