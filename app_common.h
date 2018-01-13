/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#ifndef _SWAT_WMICONFIG_COMMON_H_
#define _SWAT_WMICONFIG_COMMON_H_
#include "swat_wmiconfig_security.h"
#include "qcom_internal.h"
#include "socket.h"
#include "app_main.h"
#include "app_gpio.h"
#define SWAT_OK 1
#define SWAT_ERROR 2
#define SWAT_NOFOUND 0
/* Timer */
#define swat_time_get_ms    time_ms
#define swat_time_init      qcom_timer_init
#define swat_time_start     qcom_timer_start
#define swat_time_stop      qcom_timer_stop
#define swat_time_delete    qcom_timer_delete
/* Memory */
#define swat_mem_malloc     qcom_mem_alloc
#define swat_mem_cpy        A_MEMCPY
#define swat_mem_set        A_MEMSET
#define swat_mem_free       qcom_mem_free
/* String */
#define swat_atoi           atoi
#define swat_sscanf         A_SSCANF
#define swat_strcmp         A_STRCMP
#define swat_strcpy         A_STRCPY
/* Task */
#define swat_task_create    qcom_task_start
#define swat_task_delete    qcom_task_exit
/* TCP/IP */
#define swat_ping_request qcom_ping_request

/* Socket */
#define swat_accept         qcom_accept
#define swat_bind           qcom_bind
#define swat_close      	qcom_socket_close
#define swat_connect        qcom_connect
#define swat_setsockopt     qcom_setsockopt
#define swat_getsockopt     qcom_getsockopt
#define swat_listen         qcom_listen
#define swat_low_latency    qcom_low_latency
#define swat_recv           qcom_recv
#define swat_recvfrom       qcom_recvfrom
#define swat_send           qcom_send
#define swat_sendto         qcom_sendto
#define swat_socket         qcom_socket
#define swat_select         qcom_select
#define swat_setsockopt     qcom_setsockopt
#define swat_getsockopt     qcom_getsockopt
#define swat_fd_zero        FD_ZERO
#define swat_fd_set         FD_SET
#define swat_fd_isset       FD_ISSET
#define swat_fd_clr         FD_CLR
extern int app_printf(const char * format, ...);
extern A_UINT8 currentDeviceId;
void swat_time();
#define HTONS htons
#define HTONL htonl
#define NTOHL ntohl
#define NTOHS ntohs
#define ORG_PTR(fmt,args...) do{if(1) {app_printf(fmt,##args);}}while(0)
#define SWAT_PTF(fmt,args...) do{if(1) {swat_time();app_printf(fmt,##args);}}while(0)
#define SWAT_PTF_NO_TIME(fmt,args...) do{if(1) {app_printf(fmt,##args);}}while(0)
#define IPV4_PTF(addr) SWAT_PTF("%d.%d.%d.%d", (addr) >> 24 & 0xFF, (addr) >> 16 & 0xFF, (addr) >> 8 & 0xFF, (addr) & 0xFF)
#define IPV4_ORG_PTF(addr) ORG_PTR("%d.%d.%d.%d", (addr) >> 24 & 0xFF, (addr) >> 16 & 0xFF, (addr) >> 8 & 0xFF, (addr) & 0)
/* QCA Calc Define */
#define CALC_TIME_DEF   0
#define CALC_BYTES_DEF  0
#define CALC_KBYTES_DEF 0
#define CALC_STREAM_NUMBER 			4
#define CALC_STREAM_NUMBER_INVALID 0xFF

/*  QCA Stream Para */
#define STREAM_IP_ADDRESS_DEF  0
#define STREAM_PORT_DEF        0
#define STREAM_PKT_SIZE_DEF    64
#define STREAM_SECONDS_DEF     10
#define STREAM_NUM_OF_PKTS_DEF 10000
#define STREAM_MODE_DEF        TEST_MODE_TIME
#define STREAM_PROTOCOL_DEF    TEST_PROT_TCP
#define STREAM_DIRECTION_DEF   TEST_DIR_TX
#define STREAM_DELAY_DEF        0
#define STREAM_RAW_IP_HDR_INC   0
#define MAX_SOCLIENT 			1//2
#define DEFAULT_FD_INDEX 		0
#define SWAT_PTR_NULL_CHK(para) do {if (NULL == (para)) return;} while(0)

#define CFG_PACKET_SIZE_MAX_TX 1576
#define CFG_PACKET_SIZE_MAX_RX 1556
#define IPV4_HEADER_LENGTH     20
#define ATH_IP_PROTO_RAW    255

/* IP header's structure */
typedef struct ipheader_s {
        A_UINT8      iph_ihl:4, iph_ver:4;
        A_UINT8      iph_tos;
        A_UINT16     iph_len;
        A_UINT16     iph_ident;
        A_UINT16     iph_offset;
        A_UINT8      iph_ttl;
        A_UINT8      iph_protocol;
        A_UINT16     iph_chksum;
        A_UINT32     iph_sourceip;
        A_UINT32     iph_destip;
}ip_header;

typedef enum
{
    TEST_IPSTACK,
    TEST_UART,
    TEST_IPSTACK_ECHO,
} TEST_TYPE_e;

typedef enum
{
    TEST_PROT_TCP,
    TEST_PROT_UDP,
    TEST_PROT_RAW,
    TEST_PROT_SSL,
} TEST_PROTOCOL_e;

typedef enum
{
    TEST_DIR_TX,
    TEST_DIR_RX,
} TEST_DIRECTION_e;

typedef enum
{
    TEST_MODE_TIME,
    TEST_MODE_PACKETS,
    TEST_MODE_PRESSURE
} TEST_MODE_e;
typedef enum
{
    SERVICE_MODE,
    CLIENT_MODE,
} DEVICE_MODE_e;

typedef enum
{
	PINIT_STAT = 0,
	PDISCON_STAT,
	PCONN_STAT,
	PLOGIN_STAT,
	PTRAN_STAT,
	PLOGOUT_STAT,
}PROCSTAT;
typedef struct
{
    A_UINT32 ipAddress;           //ip address
    IP6_ADDR_T ip6Address;       //IP v6 address
    A_UINT32 mcAddress;           //multicast address for traffic receive only
    IP6_ADDR_T mcastIp6;
    IP6_ADDR_T localIp6;
    A_UINT32 port;                //port
    A_UINT32 pktSize;             //packet size
    A_UINT8* pktBuff;             //packet buffer
    A_UINT32 seconds;             //time seconds
    A_UINT32 numOfPkts;           //number of packets
    A_UINT32 delay;				//delay in ms
    TEST_MODE_e mode;           //time or packets or pressure
    TEST_PROTOCOL_e protocol;   //tcp or udp or raw
    TEST_DIRECTION_e direction; //tx or rx
    TEST_TYPE_e type;           //type
    A_UINT32 local_ipAddress;      //for raw tx
    A_UINT32 ip_hdr_inc;           //for raw tx - optional
    A_UINT32 rawmode;              //for raw rx ETH/IP mode
    A_UINT8 mcast_enabled;
    A_UINT8 is_v6_enabled;
    A_UINT32 ssl_inst_index;
    A_UINT8  iperf_mode;
    A_UINT16 iperf_display_interval;
    A_UINT16 iperf_udp_buf_size;
    A_UINT32 iperf_stream_id;
    A_UINT32 iperf_time_sec;
    A_UINT32 iperf_udp_rate;
    //----------------------
    DEVICE_MODE_e devMode;
    PROCSTAT	procStat;

} STREAM_PARA_t;

typedef struct
{
    A_UINT32 milliseconds;
} TIME_STRUCT_t;

typedef struct
{
    TIME_STRUCT_t firstTime;
    TIME_STRUCT_t lastTime;
    A_UINT32 bytes;
    A_UINT32 kbytes;
} STREAM_CALC_t;

typedef struct
{
    A_UINT32 index;
    A_INT32  socketLocal;
    A_INT32  clientFd[MAX_SOCLIENT];//for tcp rx
    STREAM_PARA_t param;
    STREAM_CALC_t calc[MAX_SOCLIENT];//for tcp rx
    void *pfd_set;
} STREAM_CXT_t;
typedef struct
{
    A_UINT8  isUsed;
} STREAM_INDEX_t;

 typedef struct
{
   A_INT32 code;
   A_INT32 packetCount;
} EOT_PACKET_t;

typedef struct
{
    A_UINT32 bytes;
    A_UINT32 kbytes;
    A_UINT32 msec;
    A_UINT32 numPackets;
} STAT_PACKET_t;

extern void swat_get_first_time(STREAM_CXT_t* pCxtPara, A_UINT32 index);
extern void swat_get_last_time(STREAM_CXT_t* pCxtPara, A_UINT32 index);
extern void swat_bytes_calc(STREAM_CXT_t* pCxtPara, A_UINT32 bytes, A_UINT32 index);
extern A_UINT32 swat_bench_quit();
extern void swat_bench_quit_init();
extern void swat_bench_quit_config();
extern void swat_buffer_free(A_UINT8** pDataBuffer);
extern void swat_socket_close(STREAM_CXT_t* pCxtPara);
extern A_UINT8 swat_cxt_index_find(STREAM_INDEX_t* pStreamIndex);
extern void swat_cxt_index_configure(STREAM_INDEX_t* pStreamIndex);
extern void swat_cxt_index_free(STREAM_INDEX_t* pStreamIndex);
extern void swat_database_initial(STREAM_CXT_t* pCxtPara);
extern void swat_database_set(STREAM_CXT_t* pCxtPara,
                                    A_UINT32 ipAddress,
                                    IP6_ADDR_T* ip6Address,
                                    A_UINT32 mcAddress,
                                    IP6_ADDR_T* localIp6,
                                    IP6_ADDR_T* mcastIp6,
                                    A_UINT32 port,
                                    A_UINT32 protocol,
				    A_UINT32 ssl_inst_index,
                                    A_UINT32 pktSize,
                                    A_UINT32 mode,
                                    A_UINT32 seconds,
                                    A_UINT32 numOfPkts,
                                    A_UINT32 direction,
                                    A_UINT32 local_ipAddress,
                                    A_UINT32 ip_hrd_inc,
                                    A_UINT32 rawmode,
                                    A_UINT32 delay, A_UINT32 iperf_mode, A_UINT32 interval, A_UINT32 udpRate);
int Inet6Pton(char * src, void * dst);
char * inet6_ntoa(char* addr, char * str);
extern int v6_enabled;
extern STREAM_CXT_t cxtTcpTxPara[CALC_STREAM_NUMBER];
extern STREAM_CXT_t cxtTcpRxPara[CALC_STREAM_NUMBER];
extern STREAM_CXT_t cxtUdpTxPara[CALC_STREAM_NUMBER];
extern STREAM_CXT_t cxtUdpRxPara[CALC_STREAM_NUMBER];
extern STREAM_CXT_t cxtRawTxPara[CALC_STREAM_NUMBER];
extern STREAM_CXT_t cxtRawRxPara[CALC_STREAM_NUMBER];
extern STREAM_INDEX_t tcpTxIndex[CALC_STREAM_NUMBER];
extern STREAM_INDEX_t tcpRxIndex[CALC_STREAM_NUMBER];
extern STREAM_INDEX_t udpTxIndex[CALC_STREAM_NUMBER];
extern STREAM_INDEX_t udpRxIndex[CALC_STREAM_NUMBER];
extern STREAM_INDEX_t rawTxIndex[CALC_STREAM_NUMBER];
extern STREAM_INDEX_t rawRxIndex[CALC_STREAM_NUMBER];
extern A_UINT32 _inet_addr(A_CHAR *str);
extern A_CHAR *_inet_ntoa(A_UINT32 ip);
extern	int Inet6Pton(char * src, void * dst);
extern char * print_ip6(IP6_ADDR_T * addr, char * str);
#endif
