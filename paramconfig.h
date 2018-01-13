/*
 * Copyright   : Fortune Tech Group. ..
 * Description : NVRAM parameter library
 * Created     : 2014-08-26 by Oliverzhang@fortune-co.com
 **/

#ifndef _PARAMCFG_H_
#define _PARAMCFG_H_

#include <qcom/qcom_common.h>
#include "wiDSEmul.h"


#define IP_ADDR(n0,n1,n2,n3) \
	(((u32)(n0) << 24) | ((u32)(n1) << 16) | ((u32)(n2) << 8) | (u8)(n3))

#define FLASH_CFG_DSETID 	0x5577
#define DSET_DATA_MAX_LEN 	512

#define DATA_PORT        	20002
#define CFG_PORT         	20001
#define UDP_PORT         	20000

//------spiHz-----------------
#define SPI_SPD_HIGH		2		// 8M
#define SPI_SPD_NORMAL		1		// 4M
#define SPI_SPD_LOW			0		// 1M

//static TX_MUTEX g_param_mutex;
struct param_ip_config 
{
	uint32_t static_ip;
	uint32_t static_submask;
	uint32_t static_gateway;
	uint8_t enable_dhcp;
	uint8_t resv[3];
};
struct sys_config
{
	//struct param_ip_config ip_config;
	//local ip
	uint32_t host_ip;	//
	uint32_t host_submask;
	uint32_t host_gateway;

	// remote ip
	A_UINT32 server_ip;
	//dns_service ip
	uint32_t dns_ip;
	uint32_t resv_dns_ip;
	//net port
	uint16_t data_port;
	uint16_t config_port;
	uint16_t udp_port;
	//dascom info
	char URL[128];
	char DeviceType[32];
	// remote ap
	char conn_ap_ssid[33];
	char conn_ap_password[65];
	uint8_t enable_dhcp;
	uint8_t recnnt_interval;	//second
	//local app
	char ap_ssid[22];
	char ap_password[16];
	// sys info
	char SN[16];
	uint8_t wifi_mode;
	uint8_t SpiHz;
	uint8_t channel;
//	user info
	char user_logName[32];
	char user_psw[16];
};

typedef struct _SYS_CONFIG
{
	DEVCFG_BASE base;
	struct sys_config cfgInfo;
}UW_SYSCFG;

extern struct sys_config g_sys_config;

void loadSysCfg(struct sys_config *p);
A_UINT16 ReadDevConfig(UW_SYSCFG *p);
A_UINT8 WriteDevConfig(const UW_SYSCFG *p);

#endif


