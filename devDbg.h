#ifndef _DEBUG_H_1_
#define _DEBUG_H_1_

#include "qcom_common.h"
#include "app_common.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define _DEBUG

#ifdef _DEBUG

//#define USE_FULL_ASSERT
//void uart1_init();

void DBG_hexs(uint8_t *dat,uint16_t len,uint8_t newLine);
void DBG_str(const char *lpS);
void DBG_dats(uint8_t *dat,uint16_t len);
void DBG_hexs_1(uint8_t *dat,uint16_t len,uint8_t newLine);

extern int cmnos_printf(const char * format, ...);

#define DBG_TRACE(fmt,args...)		do{if(1) {cmnos_printf(fmt,##args);}}while(0)
#define DBG_HEX(lpD,len) 		DBG_hexs(lpD,len,1)
#define DBG_HEX_ADDR(lpD,len) 	DBG_hexs_1(lpD,len,1)
#define DBG_STR(lpS)			DBG_str(lpS)
#define	DBG_DATS(lpD,len)		DBG_dats(lpD,len)
#define PUT_CHAR(c)				DBG_Putc(c)
#define GET_CHAR(c)				DBG_Getc(c)
#define OPEN_DEBUG()			//uart1_init()

#define DBG_STR_ONCE(old,lab,lpS)	{\
	if((A_UINT32)(old) != (A_UINT32)(lab))	\
	{\
		DBG_str(lpS);	\
		(old) = (A_UINT32)(lab);	\
	}\
}
#else
#define DBG_TRACE(fmt,...)
#define DBG_HEX_STR(lpD,len) 
#define DBG_STR(lpS)
#define	DBG_DATS(lpD,len)
#define PUT_CHAR(c)
#define GET_CHAR(c)
#define OPEN_DEBUG()
#endif

#endif
