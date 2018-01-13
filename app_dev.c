/**
 ****************************************************************************************
 *
 * @file uart.c
 *
 * @brief UART Driver for QCA4010.
 *
 * Copyright (C) Quintic 2017
 *
 * $Rev: 1.0 $
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "ringBuf.h"
#include "qcom_uart.h"
#include "select_api.h"
#include "qcom/socket_api.h"
#include "app_common.h"
#include "app_main.h"
#include "HCtrlProto.h"
#include "app_dev.h"
#include "app_gpio.h"
#include "paramconfig.h"
A_UINT8 gWifiRxBuf[wifi_buffer_size];
RING_BUFF gwrxBuf;
static void spi_rx_buf_reset(void)
{
	gwrxBuf.ringBuf = gWifiRxBuf;
	gwrxBuf.len = wifi_buffer_size;
	gwrxBuf.inIdx = 0;
	gwrxBuf.outIdx = 0;
}
A_UINT32 app_spi_tx(A_UINT8*data, A_UINT32 len)
{
	A_UINT16 idx = 0;
	A_UINT32 SendBytes=0;
	A_UINT32 lp[2];
	while (idx < len)
	{
		if (!qcom_gpio_pin_get(DEV_SPI_RTS_PIN))
		{
			SendBytes=((idx+8)<=len)?8:(len-idx);
			memcpy(lp,data+idx,SendBytes);
			qcom_spi_request(lp, 0, SendBytes );
			idx += SendBytes;
		}else break;
	}
	return idx;
}
A_UINT32 put_data_spi_tx_buf(A_UINT8* data,A_UINT32 len)
{

	if(!rngBufSpace(&gwrxBuf))
		return 0;
	else
		return putRngBufDat(&gwrxBuf, data, rngBufSpace(&gwrxBuf));
}
void app_spi_tx_proc(void)
{
	A_UINT32 length, SpiSendByte=0;
	A_UINT8 *lp = getRngDatBuf(&gwrxBuf,&length);
	if(!length)
		return;
	else
		SpiSendByte = app_spi_tx(lp, length);
	if (SpiSendByte)
		updateRngBufDats(&gwrxBuf, -SpiSendByte);
	else
		return;
}
A_STATUS spi_port_open(void)
{
	A_STATUS retVal;
	spi_rx_buf_reset();
	switch(g_sys_config.SpiHz)
	{
		case SPI_SPD_LOW:
			DBG_STR("Spi 1Mhz\r\n");
			retVal=qcom_spi_init(DEV_SPI_SEL_PIN, DEV_SPI_MOSI_PIN, DEV_SPI_MISO_PIN,DEV_SPI_CLK_PIN, SPI_FREQ_800K);
			break;
		case SPI_SPD_NORMAL:
			DBG_STR("Spi 4Mhz\r\n");
			retVal=qcom_spi_init(DEV_SPI_SEL_PIN,DEV_SPI_MOSI_PIN,DEV_SPI_MISO_PIN,DEV_SPI_CLK_PIN,SPI_FREQ_3250K);
			break;
		case SPI_SPD_HIGH:
			DBG_STR("Spi 8Mhz\r\n");
			retVal=qcom_spi_init(DEV_SPI_SEL_PIN,DEV_SPI_MOSI_PIN,DEV_SPI_MISO_PIN,DEV_SPI_CLK_PIN,SPI_FREQ_6500K);
			break;
		default:
			DBG_STR("Spi 1Mhz\r\n");
			retVal=qcom_spi_init(DEV_SPI_SEL_PIN,DEV_SPI_MOSI_PIN,DEV_SPI_MISO_PIN,DEV_SPI_CLK_PIN,SPI_FREQ_800K);
			break;
	}
	return retVal;
}

typedef struct _UART_PARAM
{
	char UartNumber[10];
	A_INT32 uart_fd;
	qcom_uart_para uart_para;
}UART_PARAM;
UART_PARAM uart_info;
A_INT32 syncSendDevPkt(HCTRL_CMDPKT *lpPkt)
{
	A_UINT32 n;
	A_INT32 retVal = -1;

	A_UINT16 len,outIdx,inIdx,cnt;
	A_UINT8 m,code;

	static A_UINT8 devBusy = 0;

	if(uart_info.uart_fd ==-1)
		return -1;

	if(devBusy)
	{
		DBG_STR("syncSendDevPkt : Device Busy ...\r\n");
		return -4;
	}
	devBusy = 1;
	code = lpPkt->Code;
	len = HCTRL_CMDGET_PKTLEN(*lpPkt);
	outIdx = inIdx = 0;
	retVal = -2;
	cnt = 3;
	while(cnt-- && outIdx < len)
	{//send to device
		n = len - outIdx;
		qcom_uart_write(uart_info.uart_fd, (A_CHAR*) &lpPkt->buf[outIdx], &n);
		if( !n )
		{
			qcom_thread_msleep(20);
			continue;
		}
		outIdx += n;
	}

	if(outIdx < len) //send timeout
		goto SYNCSEND_EXIT;

	retVal = -3;
	cnt = 100;
	while(cnt--)
	{
		qcom_thread_msleep(10);
		//tx_thread_sleep(1);
		while(inIdx < HCTRL_CMDPKT_HDLEN)
		{
			n = 1;
			qcom_uart_read(uart_info.uart_fd, (A_CHAR*)&lpPkt->buf[inIdx], &n);
			if( !n )
				break;

			if(lpPkt->buf[0] == HCTRL_PKT_ID)
				inIdx++;
		}

		while(inIdx < HCTRL_CMDGET_PKTLEN(*lpPkt))
		{
			n = HCTRL_CMDGET_PKTLEN(*lpPkt) - inIdx;
			qcom_uart_read(uart_info.uart_fd, (A_CHAR*)&lpPkt->buf[inIdx], &n);
			if( !n )
				break;

			inIdx += n;
		}

		if( inIdx < HCTRL_CMDGET_PKTLEN(*lpPkt))
			continue;

		if(code != lpPkt->Code)
		{
			inIdx = 0;	//ignore
			cnt = 20;
			continue;
		}

		retVal = HCTRL_CMDGET_PKTLEN(*lpPkt);
		if(lpPkt->Len)
		{
			m = 0;
			for (n = 0; n < lpPkt->Len; n++)
				m += lpPkt->dats[n];

			if(lpPkt->Sum != m)
				retVal = -4;
		}
		break;
	}

SYNCSEND_EXIT:

	devBusy = 0;
	return retVal;	//recv timeout
}

A_INT16 getDevRspInfo(A_UINT8 hCtrl,DEV_REQPKT *lpReq,A_UINT8 *lpRsqBuf,A_UINT16 *bufLen)
{
	A_INT32 m;
	HCTRL_PTOKEN *lpToken = getCtrlToken(hCtrl);
	A_UINT16 len = *bufLen;

	*bufLen = 0;
	if(!lpToken)
		return -1;

	mkHCtrlPkt(&lpToken->cmdPkt,lpReq->code,lpReq->param,lpReq->pLen);
	m = syncSendDevPkt(&lpToken->cmdPkt);
	if(m <= 0)
	{
		DBG_TRACE("getDevRspInfo : syncSendDevPkt fail(%d)\r\n",m);
		return -2;
	}

	if(lpToken->cmdPkt.Len)
	{
		m = HCTRL_CMDGET_PKTLEN(lpToken->cmdPkt);
		if(lpRsqBuf && m < len)
		{
			memcpy(lpRsqBuf,lpToken->cmdPkt.buf,m);
			*bufLen = m;
		}
		return 0;
	}
	return lpToken->cmdPkt.Sum;
}
static void app_uart_param_set(UART_PARAM* para)
{
	para->uart_fd=-1;
	para->uart_para.BaudRate = 115200;
	para->uart_para.number = 8;
	para->uart_para.parity = 0;
	para->uart_para.StopBits = 1;
	para->uart_para.FlowControl = 1;
	memset(para->UartNumber,0,sizeof(para->UartNumber));
	memcpy(para->UartNumber,"UART1",sizeof("UART1"));
	qcom_uart_set_buffer_size((A_CHAR *)para->UartNumber,0, uart_buffer_size);
	qcom_set_uart_config((A_CHAR *)para->UartNumber, &para->uart_para);
}
A_INT32 uart_port_open(void)
{
	if(qcom_uart_init() >= 3)
	{
		app_uart_param_set(&uart_info);
		uart_info.uart_fd = qcom_uart_open((A_CHAR *) uart_info.UartNumber);
	}else
		uart_info.uart_fd=-1;
	if(uart_info.uart_fd<0)
		return A_ERROR;
	else
		return A_OK;
}
A_INT32 uart_close(void)
{
	if (uart_info.uart_fd != -1)
	{
		return qcom_uart_close(uart_info.uart_fd);
	}
	return -1;
}


