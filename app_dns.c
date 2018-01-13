/*
 * app_dns.c

 *
 *  Created on: Dec 26, 2017
 *      Author: root
 */
#include "app_dns.h"
#include "qcom_common.h"
#include "qcom/socket_api.h"
#include "app_common.h"
A_STATUS app_dns_set_hostname(A_CHAR *host_name)
{
    SWAT_PTF("Change the DHCP hostname to be : %s \n", host_name);
    return qcom_set_hostname(0,host_name);
}

A_STATUS app_dns_svr_del(A_CHAR *ipaddr)
{
    A_UINT32 ip = 0;
    A_UINT8 dnsSerIp[16];
    A_MEMSET(dnsSerIp, 0, 16);
    if(!Inet6Pton(ipaddr, dnsSerIp))
    {
       return  qcom_dnsc_del_server_address(dnsSerIp, AF_INET6);
    } else if (0 != (ip = _inet_addr(ipaddr)))
    {
        A_MEMCPY(dnsSerIp, (char*)&ip, 4);
       return  qcom_dnsc_del_server_address(dnsSerIp, AF_INET);
    } else
    {
        SWAT_PTF("input ip addr is not valid!\n");
    }
    return A_OK;
}
A_STATUS app_dns_set_local_domain(A_CHAR *local_name)
{
    if(qcom_dns_local_domain(local_name)==A_OK)
	{
    	SWAT_PTF("set DNS local domain name as %s\n", local_name);
    	return A_OK;
	}
    return A_ERROR;
}
A_STATUS app_dns_resolvehostbyname(DNS_GET_PARAM *dns)//just one name
{
    A_UINT32 ipAddress;
    if(qcom_dnsc_get_host_by_name(dns->name, &ipAddress)== A_OK)
    {
        A_CHAR *ipaddr;
        dns->ipv4=ipAddress;
        ipaddr = _inet_ntoa(ipAddress);
        SWAT_PTF("Get IP address of host %s \n", dns->name);
        SWAT_PTF("ip address is %s\n", (char *)_inet_ntoa(ipAddress));
    }else
    {
        SWAT_PTF("The IP of host %s is not gotten\n", dns->name);
        return A_ERROR;
    }
    return A_OK;
}
A_STATUS app_dns_resolvehostbyname2(DNS_GET_PARAM *dns)
{
   IP6_ADDR_T ip6Addr;
   char ip6buf[48];
   char *ip6ptr =  NULL;
   A_UINT32 ipAddress;
   A_CHAR *ipaddr;
   A_STATUS ret;
   ret = qcom_dnsc_get_host_by_name2(dns->name, (A_UINT32*)(&ip6Addr), dns->domain,  dns->mode);
   if(ret!=A_OK)
   {
        SWAT_PTF("The IP of host %s is not gotten\n", dns->name);
        return A_ERROR;
   }
   SWAT_PTF("Get IP address of host %s \n", dns->name);
   if (dns->domain == AF_INET6)
   {
        ip6ptr = print_ip6(&ip6Addr, ip6buf);
        SWAT_PTF("ip address is %s\n", ip6buf);
        dns->ipv6=ip6Addr;
    } else
    {
        memcpy(&ipAddress, &ip6Addr, 4);
        ipAddress = ntohl(ipAddress);
        dns->ipv4=ipAddress;
        ipaddr = _inet_ntoa(ipAddress);
        SWAT_PTF("ip address is %s\n", ipaddr);
    }
   return A_OK;
}

A_STATUS app_dns_ip_get_mult_dns_ip(DNS_GET_PARAM *dns)
{
    IP6_ADDR_T ip6Addr;
    char ip6buf[48];
    char *ip6ptr =  NULL;
    A_UINT32 ipAddress;
    A_CHAR *ipaddr;
    dns->dns_tmp = NULL;
    int i = 0;
	dns->dns_tmp = qcom_dns_get(dns->mode, dns->domain, dns->name); //mode: 0-GETHOSTBYNAME; 1-GETHOSTBYNAME2; 2-RESOLVEHOSTNAME
	if (NULL == dns->dns_tmp)
	{
		qcom_printf("Can't get IP addr by host name %s\n", dns->name);
		return A_ERROR;
	}
	SWAT_PTF("Get IP address of host %s \nip address is", dns->name);
	if (dns->domain == AF_INET6)
	{
		for (; i < MAX_SUPPORT_DNS; i++)
		{
			if (dns->dns_tmp->h_addr_list[i])
			{
				memcpy(&ip6Addr, dns->dns_tmp->h_addr_list[i], 16);
				ip6ptr = print_ip6(&ip6Addr, ip6buf);
				SWAT_PTF("  %s", ip6buf);
			}
		}
		SWAT_PTF("\n");
	} else
	{
		for (i = 0; i < MAX_SUPPORT_DNS; i++)
		{
			if (dns->dns_tmp->h_addr_list[i])
			{
				memcpy(&ipAddress, dns->dns_tmp->h_addr_list[i], 4);
				ipAddress = ntohl(ipAddress);
				ipaddr = _inet_ntoa(ipAddress);
				SWAT_PTF("  %s", ipaddr);
			}
		}
		SWAT_PTF("\n");
	}
	return A_OK;
}


A_STATUS app_dns_svr_add(A_CHAR *ipaddr)
{
    A_UINT32 ip = 0;
    A_UINT8 dnsSerIp[16];
    DnsSvrAddr_t dns[MAX_SUPPORT_DNS];
	A_UINT32 number = 0;
	A_UINT32 i = 0;
    A_MEMSET(dnsSerIp, 0, 16);
    A_MEMSET(dns,0,MAX_SUPPORT_DNS*sizeof(DnsSvrAddr_t));
    qcom_dns_server_address_get(dns, &number);
    if(!Inet6Pton(ipaddr, dnsSerIp))
    {
		for ( ; i < number; i++)
		{
			if ((dns[i].type == AF_INET6) && (!memcmp(dns[i].addr6, dnsSerIp, 16)))
			{
				return A_OK;
			}
		}
       return  qcom_dnsc_add_server_address(dnsSerIp, AF_INET6);
    } else if (0 != (ip = _inet_addr(ipaddr)))
    {
		for ( ; i < number; i++)
		{
			if ((dns[i].type == AF_INET) && (dns[i].addr4 == ip))
			{
				return A_OK;
			}
		}
        A_MEMCPY(dnsSerIp, (char*)&ip, 4);
        SWAT_PTF("add dns service ip:%s\r\n",_inet_ntoa(ip));
       return  qcom_dnsc_add_server_address(dnsSerIp, AF_INET);
    } else
    {
        SWAT_PTF("input ip addr is not valid!\n");
    }
    return A_OK;
}
A_STATUS app_dns_entry_add(A_CHAR* hostname, A_CHAR *ipaddr)
{
    A_UINT32 ip = 0;
	A_UINT8 ipv6[16];
	A_MEMSET(ipv6, 0, 16);
	if(!Inet6Pton(ipaddr, ipv6))
	{
		SWAT_PTF("input ip addr is V6!\n");
		return qcom_dns_6entry_create(hostname, ipv6);
	}
	else if(0 != (ip = _inet_addr(ipaddr)))
	{
        SWAT_PTF("input ip addr is V4!\n");
		return qcom_dns_entry_create(hostname, ip);
    }else
	{
        SWAT_PTF("input ip addr is not valid!\n");
	}
	return A_OK;
}
A_STATUS app_dns_entry_del(A_CHAR* hostname, A_CHAR *ipaddr)
{
    A_UINT32 ip = 0;
	A_UINT8 ipv6[16];
	A_MEMSET(ipv6, 0, 16);

    if(!Inet6Pton(ipaddr, ipv6))
    {
		SWAT_PTF("input ip addr is V6!\n");
		return qcom_dns_6entry_delete(hostname, ipv6);
	}
	else if (0 != (ip = _inet_addr(ipaddr)))
	{
        SWAT_PTF("input ip addr is V4!\n");
		return qcom_dns_entry_delete(hostname, ip);
    }
	else
	{
        SWAT_PTF("input ip addr is not valid!\n");
	}
    return A_OK;
}

A_STATUS app_set_dns_timeout(DNS_GET_PARAM *dns)
{
    return qcom_dns_set_timeout(dns->timeout);
}
A_STATUS app_dns_client_enable(DNS_GET_PARAM *dns)
{
	A_UINT8 flag =(dns->clientEn>0)?1:0;
    /* 1: enable; 0:diable; */
    return qcom_dnsc_enable(flag);
}
A_STATUS app_dns_service_enable(DNS_GET_PARAM *dns)
{
  A_UINT8 flag =(dns->serviceEn>0)?1:0;
  return qcom_dnss_enable(flag);
}
void app_dns_para_set(DNS_GET_PARAM *dns,A_CHAR *DnsAdr,int Timeout,A_CHAR* ResDnsAdr,CHAR *Name,
						A_INT32 Domain,CHAR *ResvName,A_UINT32 Ipv4,IP6_ADDR_T Ipv6,A_UINT8 ClientEn,
						A_UINT8 ServiceEn,A_UINT32 Mode,struct hostent *Dns_tmp)
{
	dns->clientEn=ClientEn;
	dns->dnsAdr=DnsAdr;
	dns->dns_tmp=Dns_tmp;
	dns->domain=Domain;
	dns->ipv4=Ipv4;
	dns->ipv6=Ipv6;
	dns->mode=Mode;
	dns->name=Name;
	dns->resDnsAdr=ResDnsAdr;
	dns->resvname=ResvName;
	dns->serviceEn=ServiceEn;
	dns->timeout=Timeout;
}
A_STATUS app_dns_proc(DNS_GET_PARAM *dns)
{
	SWAT_PTF("dns start\r\n");
	SWAT_PTF("dns1:%s\r\n",dns->dnsAdr);
	SWAT_PTF("dns2:%s\r\n",dns->resDnsAdr);
	if(app_set_dns_timeout(dns)!=A_OK)
		return A_ERROR;
	SWAT_PTF("1\r\n");
	if(app_dns_client_enable(dns)==A_OK)
	{
		SWAT_PTF("2\r\n");
		if(app_dns_svr_add(dns->dnsAdr)!=A_OK)
			return A_ERROR;
		SWAT_PTF("3\r\n");
		if(app_dns_svr_add(dns->resDnsAdr)!=A_OK)
			return A_ERROR;
		SWAT_PTF("4\r\n");
		if(app_dns_resolvehostbyname2(dns)!=A_OK)
			return A_ERROR;
		SWAT_PTF("dns client start\r\n");
		return A_OK;
	}
	if(app_dns_service_enable(dns)==A_OK)
	{
		if(app_dns_set_hostname("DascomWifi")!=A_OK)
			return A_ERROR;
		SWAT_PTF("5\r\n");
		if(app_dns_entry_add("DascomWifi", "192.168.1.10")!=A_OK)
			return A_ERROR;
		SWAT_PTF("6\r\n");
		SWAT_PTF("dns Service start\r\n");
		return A_OK;
	}
	return A_ERROR;
}




