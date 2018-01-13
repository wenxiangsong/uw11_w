
#ifndef _RINGBUF_H
#define _RINGBUF_H
#include "qcom_common.h"

#define RING_BUF_EMPTY(p)		((p)->inIdx == (p)->outIdx)
#define RING_BUF_UNEMPTY(p)		((p)->inIdx != (p)->outIdx)
#define RING_BUF_ISFULL(p)		((((p)->inIdx + 1) & ((p)->len - 1)) == (p)->outIdx)
#define RNG_EVENT_RX_FULL	1		//数据接收将满
#define RNG_EVENT_RX_OUT	2		//有数据被取出
#define RNG_EVENT_TX_IN		3		//有数据到发送缓冲
#define RNG_EVENT_LOCK		4		//
#define RNG_EVNET_UNLOCK	5

struct _RING_BUFF;
typedef A_INT32 (*RNG_EVENT)(struct _RING_BUFF *lpRngBuf,A_UINT8 eventID, void *param);

typedef struct _RING_BUFF{	
	
	A_UINT8 *ringBuf;

	A_UINT32 len;	//需为 0x1000,0x2000
		
	A_UINT32 inIdx;
	A_UINT32 outIdx;
		
}RING_BUFF, * LPRING_BUFF;

static inline A_UINT32 getRngBufDat(LPRING_BUFF lpRngBuf,A_UINT8 *d,A_UINT32 len)
{
	A_UINT32 idx = 0;
	while(len--)
	{
		if(RING_BUF_UNEMPTY(lpRngBuf))
		{
			*d++ = lpRngBuf->ringBuf[lpRngBuf->outIdx++];
			lpRngBuf->outIdx &= (lpRngBuf->len - 1);
			idx++;
		}else
			break;
	}
	return idx;
}

static inline A_UINT32 putRngBufDat(LPRING_BUFF lpRngBuf,A_UINT8 *d,A_UINT32 len)
{
	A_UINT32 idx = 0;
	
	while(len--)
	{
		if(!RING_BUF_ISFULL(lpRngBuf))
		{
			lpRngBuf->ringBuf[lpRngBuf->inIdx++] = *d++;
			idx++;
			lpRngBuf->inIdx &= (lpRngBuf->len - 1);
		}else
			break;
	}
	return idx;
}

static inline A_UINT32 rngBufDatCnts(LPRING_BUFF lpRngBuf)
{
	A_INT32 n;
	
	n = lpRngBuf->inIdx - lpRngBuf->outIdx;
	if( n < 0)
		n += lpRngBuf->len;
		
	return n;
}
static inline A_UINT32 rngBufSpace(LPRING_BUFF lpRngBuf)
{
	return lpRngBuf->len - rngBufDatCnts(lpRngBuf) - 1;
}
static inline A_UINT8* getRngDatBuf(LPRING_BUFF lpRngBuf,A_UINT32 *len)
{	
	A_UINT32 n;

	n = lpRngBuf->inIdx;		//inIdx 会发生变化

	if(lpRngBuf->outIdx <= n)//lpRngBuf->inIdx)
		*len = n - lpRngBuf->outIdx;//lpRngBuf->inIdx - lpRngBuf->outIdx;
	else
		*len = lpRngBuf->len - lpRngBuf->outIdx;
	

	return &lpRngBuf->ringBuf[lpRngBuf->outIdx];
}

static inline A_UINT8* getRngSpaceBuf(LPRING_BUFF lpRngBuf,A_UINT32 *len)
{	
	A_UINT32 in,out;

	*len = 0;
						//lock uart irq
	if(!RING_BUF_ISFULL(lpRngBuf))
	{
		in = lpRngBuf->inIdx;
		out = lpRngBuf->outIdx;
		if(out <= in)
			*len = lpRngBuf->len - in ;
		else
			*len = out - in;

		if( *len > 1)
			(*len)--;
	}
						//unlock uart irq
	return &lpRngBuf->ringBuf[lpRngBuf->inIdx];
}

static inline void updateRngBufDats(LPRING_BUFF lpRngBuf,A_INT32 len)
{
    if( len < 0)
    {
      len = 0 - len;
      lpRngBuf->outIdx +=  len;
      lpRngBuf->outIdx &= (lpRngBuf->len - 1);			
   }else
   {
     lpRngBuf->inIdx = (lpRngBuf->inIdx + len) & (lpRngBuf->len - 1);		
   }
}

#endif /* end _UART_H_ */



