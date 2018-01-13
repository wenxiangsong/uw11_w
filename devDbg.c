#include "devDbg.h"
#include "qcom_uart.h"

/********************************************************************/
static const char ASSERT_FAILED_STR[] = "Assertion failed in %s at line %d\n";
void assert_failed(uint8_t* file, uint32_t line)
{
    //int i;
    
    DBG_TRACE(ASSERT_FAILED_STR, file, line);

    while (1);
}

void DBG_dats(uint8_t *dat,uint16_t len)
{
	while(len >= 8)
	{
		DBG_TRACE("%c%c%c%c%c%c%c%c",dat[0],dat[1],dat[2],dat[3],
					dat[4],dat[5],dat[6],dat[7]);
		dat += 8;
		len -= 8;
	}

	while(len--)
	{
		DBG_TRACE("%c",*dat++);
	}
}
void DBG_str(const char *lpS)
{
	DBG_TRACE("%s",lpS);
}

static const uint8_t hexChar[]= {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
void DBG_hexs(uint8_t *dat,uint16_t len,uint8_t newLine)
{
	uint8_t buf[51],idx;
	uint16_t i;
  
	idx = 0;
	for( i = 0; i < len; i++)
	{
		buf[idx++] = hexChar[dat[i] >> 4];
		buf[idx++] = hexChar[dat[i] & 0x0f];
		buf[idx++] = 0x20;
	
		if( idx >= 48)
		{
			if(newLine)
			{
				buf[idx++] = 0x0d;
				buf[idx++] = 0x0a;
			}
			DBG_dats(buf,idx);
			idx = 0;
		}
  	}
	if( idx > 0 )
	{
		if( newLine)
		{
			buf[idx++] = 0x0d;
			buf[idx++] = 0x0a;
		}
		DBG_dats(buf,idx);
	}
}

void DBG_hexs_1(uint8_t *dat,uint16_t len,uint8_t newLine)
{
	uint8_t buf[61],idx;
	uint16_t i,addr;
  
	idx = 10;
	addr = 0;
	for( i = 0; i < len; i++)
	{
		buf[idx++] = hexChar[dat[i] >> 4];
		buf[idx++] = hexChar[dat[i] & 0x0f];
		buf[idx++] = 0x20;
	
		if( idx >= 58)
		{
			buf[0] = (uint8_t)hexChar[(addr >> 20) & 0x0f];
			buf[1] = (uint8_t)hexChar[(addr >> 16) & 0x0f];
			buf[2] = (uint8_t)hexChar[(addr >> 12) & 0x0f];
			buf[3] = (uint8_t)hexChar[(addr >> 8) & 0x0f];
			buf[4] = (uint8_t)hexChar[(addr >> 4) & 0x0f];
			buf[5] = (uint8_t)hexChar[addr & 0x0f];
			buf[6] = ':';
			buf[7] = buf[8] = buf[9] = 0x20;
			if(newLine)
			{
				buf[idx++] = 0x0d;
				buf[idx++] = 0x0a;
			}
			
			DBG_dats(buf,idx);

			idx = 10;
			addr += 16;
		}
  	}
	if( idx > 10 )
	{
		buf[0] = (uint8_t)hexChar[(addr >> 20) & 0x0f];
		buf[1] = (uint8_t)hexChar[(addr >> 16) & 0x0f];
		buf[2] = (uint8_t)hexChar[(addr >> 12) & 0x0f];
		buf[3] = (uint8_t)hexChar[(addr >> 8) & 0x0f];
		buf[4] = (uint8_t)hexChar[(addr >> 4) & 0x0f];
		buf[5] = (uint8_t)hexChar[addr & 0x0f];
		buf[6] = ':';
		buf[7] = buf[8] = buf[9] = 0x20;
		if( newLine)
		{
			buf[idx++] = 0x0d;
			buf[idx++] = 0x0a;
		}

		DBG_dats(buf,idx);
	}
}

