/*
 * paramconfig.c
 *
 *  Created on: Dec 4, 2017
 *      Author: root
 */
#include <qcom/qcom_common.h>
#include <qcom/socket_api.h>
#include <qcom/select_api.h>
#include <qcom/qcom_utils.h>
#include <qcom/qcom_nvram.h>
#include <qcom/qcom_uart.h>
#include <qcom/qcom_gpio.h>
#include <qcom/qcom_i2c.h>
#include <qcom/qcom_misc.h>
#include <qcom/qcom_pwm.h>
#include <qcom/qcom_dset.h>
#include "threadx/tx_mutex.h"
#include "devDbg.h"

#include "paramconfig.h"

static const UW_SYSCFG gSysCfg_Default = {
		{
			GENCFG_DEVID,				// devID
			UWCFG_SYS_CFGID,			// cfgID
			(A_UINT8)(sizeof(UW_SYSCFG) >> 8),		// cfgLen_H
			(A_UINT8)(sizeof(UW_SYSCFG)),	// cfgLen_L
			UWCFG_SYS_CFGVER,			// cfgVer
			0,							// crc
			{0,0,},						// resv[2]
		},
		{
			IP_ADDR(192,168,1,10),	//uint32_t host_ip;
			IP_ADDR(255,255,255,0),	//uint32_t host_submask;
			IP_ADDR(192,168,1,10),	//uint32_t host_gateway;
			IP_ADDR(192,168,253,1),	//uint32_t service ip

			IP_ADDR(114,114,114,114),//uint32_t dns service ip
			IP_ADDR(8,8,8,8),		//uint32_t resv dns service ip
			// port
			DATA_PORT,				//uint16_t data_port;
			CFG_PORT,				//uint16_t config_port;
			UDP_PORT,				//uint16_t udp_port;
			//dascom info
			"yun.ifengwo.com.cn",	//char URL[128]
			"DL210",     			//char DeviceType[32];
			// remote ap
			"DASCOMWEN",			//char conn_ap_password[32]
			"1234567890",			//char conn_ap_password[65];
			1,					    //uint8_t enable_dhcp;
			3,					    //reconnect 3 second

			//local app
			"uw11_test-mice12",		//char ap_ssid[22];
			"1234567890",			//char ap_password[16];

			// sys info
			"00171211000024",		//char SN[16];
			QCOM_WLAN_DEV_MODE_AP,	//uint8_t wifi_mode
			SPI_SPD_HIGH,			//uint8_t SpiHz;
			6,						//channel

			//	user info
			"user_logName",					//char user_logName[32];
			"\0",					//char user_psw[16];
		},
};
u8 mkDSCfgPkt(DEVCFG_BASE *lpCfg,u8 *lpD,u16 len,u8 cfgID,u8 ver);
u8 chkDSCfgPkt(const DEVCFG_BASE *lpCfg);
void loadDefaultSysConfig(struct sys_config *p)
{
	*p = gSysCfg_Default.cfgInfo;
}

static int ConfigReadData(int len, unsigned char * value,unsigned short magic)
{
	A_STATUS status;
	A_UINT32 dset_handle;

	status = qcom_dset_open(&dset_handle, FLASH_CFG_DSETID, DSET_MEDIA_NVRAM,NULL, NULL);
	if (status != A_OK)
		return -1;

	if(qcom_dset_size(dset_handle) < len)
		return -2;

	qcom_dset_read(dset_handle, value, len, 0, NULL, NULL);

	qcom_dset_close(dset_handle, NULL, NULL);

	return 0;
}

static A_UINT8 ConfigWriteData(int len, unsigned char * value)
{
	A_STATUS status;
	A_UINT32 dset_handle;
	//char ret = -1;

	//tx_mutex_create(&g_param_mutex, "param_mutex", TX_INHERIT);
	DBG_STR("ConfigWriteData...\r\n");

	status = qcom_dset_open(&dset_handle, FLASH_CFG_DSETID, DSET_MEDIA_NVRAM,
			NULL, NULL);
	if (status == A_OK)
	{
		qcom_dset_close(dset_handle, NULL, NULL);
		qcom_dset_delete(FLASH_CFG_DSETID, DSET_MEDIA_NVRAM, NULL, NULL);
	}
	status = qcom_dset_create(&dset_handle, FLASH_CFG_DSETID, len,DSET_MEDIA_NVRAM, NULL, NULL);
	if (status == A_OK)
	{
		SWAT_PTF("start save %d data\r\n", len);
		qcom_dset_write(dset_handle, value, len, 0, 0, NULL, NULL);
		qcom_dset_commit(dset_handle, NULL, NULL);
		qcom_dset_close(dset_handle, NULL, NULL);

		return 0;
	} else
	{
		DBG_STR("Config Write Data Fail\r\n");
	}

	return 1;
}

A_UINT8 WriteDevConfig(const UW_SYSCFG *p)
{
	UW_SYSCFG mcfg;
	int ret;

	ret = chkDSCfgPkt((const DEVCFG_BASE *)p);
	if(ret)
	{
		DBG_TRACE("WriteDevConfig : chk pkt fail(%d)\r\n",ret);
		return 1;
	}

	if(!ConfigReadData(sizeof(mcfg), (unsigned char *)&mcfg, 0))
	{
		if(!chkDSCfgPkt((const DEVCFG_BASE *)&mcfg))
		{
			if(!memcmp((void*)&mcfg,(void*)p,sizeof(mcfg)))
			{
				DBG_STR("WriteDevConfig : same datas...\r\n");
				return 0;
			}
		}
	}

	return ConfigWriteData(sizeof(*p), (unsigned char *) p);
}

A_UINT16 ReadDevConfig(UW_SYSCFG *p)
{
	int ret;
	A_UINT16 len = sizeof(*p);

	ret = ConfigReadData(len, (unsigned char *) p, 0);
	if (!ret)// == sizeof(*p))
	{
		ret = chkDSCfgPkt((const DEVCFG_BASE *)p);
		if(!ret)
		{
			DBG_STR("ReadDevConfig Ok...\r\n");
			return len;
		}

		DBG_TRACE("Check Config Pkt Fail(%d)\r\n",ret);
	}else
	{
		DBG_TRACE("Read Device Config Error(%d)\r\n", ret);
	}

	*p = gSysCfg_Default;

	mkDSCfgPkt((DEVCFG_BASE *)p,
			(u8*)&gSysCfg_Default.cfgInfo,
			sizeof(UW_SYSCFG) - sizeof(DEVCFG_BASE),
			gSysCfg_Default.base.cfgID,
			gSysCfg_Default.base.cfgVer);

	DBG_STR("ReadDevConfig Write default config to flash...\r\n");

	WriteDevConfig(p);

	return len;
}
void loadSysCfg(struct sys_config *p)
{
	UW_SYSCFG uwCfg;

	ReadDevConfig(&uwCfg);

	*p = uwCfg.cfgInfo;
}



