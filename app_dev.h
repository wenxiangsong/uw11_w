/*
 * app_dev.h
 *
 *  Created on: Jan 4, 2018
 *      Author: root
 */

#ifndef APP_DEV_H_
#define APP_DEV_H_
#include "ringBuf.h"
#include "qcom_spi.h"
#include "ringBuf.h"
#define SPI_SPD_LOW 	0 	//1Mhz
#define SPI_SPD_NORMAL 	1 	//4Mhz
#define SPI_SPD_HIGH 	2 	//8Mhz
#define wifi_buffer_size 0x40000
#define uart_buffer_size 0x400
#define PUT_UART_TXBUF(lpD,len)	 	putRngBufDat(&gutxBuf,lpD,len)
#define PUT_SPI_TXBUF(lpD,len)		putRngBufDat(&gwrxBuf,lpD,len)
#define GET_UART_RXBUF(lpD,len)		getRngBufDat(&gurxBuf,lpD,len)
typedef struct _DEV_REQPKT{
	A_UINT8 code;
	A_UINT8 pLen;
	A_UINT8 param[16];
}DEV_REQPKT;
extern RING_BUFF gwrxBuf;
A_STATUS spi_port_open(void);
A_UINT32 app_spi_tx(A_UINT8*data, A_UINT32 len);
A_UINT32 put_data_spi_tx_buf(A_UINT8* data,A_UINT32 len);
void app_spi_tx_proc(void);
A_INT32 syncSendDevPkt(HCTRL_CMDPKT *lpPkt);
A_INT16 getDevRspInfo(A_UINT8 hCtrl,DEV_REQPKT *lpReq,A_UINT8 *lpRsqBuf,A_UINT16 *bufLen);
A_INT32 uart_port_open(void);
A_INT32 uart_close(void);
#endif /* APP_DEV_H_ */
