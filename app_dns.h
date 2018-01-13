/*
 * app_dns.h
 *
 *  Created on: Dec 26, 2017
 *      Author: root
 */

#ifndef APP_DNS_H_
#define APP_DNS_H_
#include "qcom_common.h"
#include "qcom/qcom_internal.h"
#define MAX_SUPPORT_DNS                 0x03
#define Disable                         0x00
#define Enable                          0x01
#define GETHOSTBYNAME 0x00
#define GETHOSTBYNAME2 0x01
#define RESOLVEHOSTNAME 0x02
typedef struct DNS_GET_PARAM
{
	A_UINT32 mode;
	A_INT32 domain;
	A_UINT32 ipv4;
	struct hostent *dns_tmp;
	int timeout;
	CHAR *name;
	A_CHAR *dnsAdr;
	CHAR *resvname;
	A_CHAR* resDnsAdr;
	A_UINT8 clientEn;
	A_UINT8 serviceEn;
	IP6_ADDR_T ipv6;
}DNS_GET_PARAM;
void app_dns_para_set(DNS_GET_PARAM *dns,A_CHAR *DnsAdr,int Timeout,A_CHAR* ResDnsAdr,CHAR *Name,
						A_INT32 Domain,CHAR *ResvName,A_UINT32 Ipv4,IP6_ADDR_T Ipv6,A_UINT8 ClientEn,
						A_UINT8 ServiceEn,A_UINT32 Mode,struct hostent *Dns_tmp);
A_STATUS app_dns_proc(DNS_GET_PARAM *dns);

#endif /* APP_DNS_H_ */
