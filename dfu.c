#include "dsEmulCmd.h"
#include "HCtrlProto.h"
#include "dfu.h"
#include "devDbg.h"
#include "qcom/qcom_ota_api.h"

//#define BIN_BUF_LEN	(1024)
//#pragma   pack(4)
//static u8 binBuf[BIN_BUF_LEN];
//#pragma   pack()

//#define DFU_FW      1
//#define DFU_BT		2
//#define DFU_UNKNOWN 0xFF

//#define FW_NAME "uw11_w fw "

static DFU_INFO gDFUInfo;

static void dfu_Init(void)
{
	DBG_STR("dfu_Init...\r\n");

	gDFUInfo.idx = 0;
	gDFUInfo.offset = 0;
	gDFUInfo.stat = DFU_STAT_IDLE;
	//gDFUInfo.type = DFU_UNKNOWN;
	gDFUInfo.ackCode = DFU_ACK_OK;

	qcom_ota_session_end(0);
}

static void dfu_stat_start(u8 *lpDats, u16 len)
{
	A_UINT32 partition_size;
	A_UINT32 size = 0;
	A_UINT8	i;
	//DBG_STR("dfu_stat_start...\r\n");

	if(len)
	{
		i = 0;
		size = 0;
		while(len-- && i < 4)
		{
			size = (size << 8) + *lpDats++;
		}
	}
	if (qcom_ota_session_start(QCOM_OTA_TARGET_FIRMWARE_UPGRADE, PARTITION_AUTO)
			!= QCOM_OTA_OK)
	{
		gDFUInfo.ackCode = DFU_ACK_ERR_FLSH;
		DBG_STR("dfu_stat_start...fail1\r\n");
		goto DFU_START_EXIT;
	}

	partition_size = qcom_ota_partition_get_size();
	//DBG_TRACE("partition size = %d...\r\n", partition_size);
	if (!partition_size || partition_size < size)
	{
		gDFUInfo.ackCode = DFU_ACK_ERR_FLSH;
		DBG_STR("dfu_stat_start...fail2\r\n");
		goto DFU_START_EXIT;
	}

	if(size)
	{
		DBG_TRACE("qcom_ota_partition_erase_sectors : size(%d)....\r\n",size);

		if(QCOM_OTA_OK != qcom_ota_partition_erase_sectors(size))
		{
			DBG_STR("qcom_ota_partition_erase_sectors : fail....\r\n");
			gDFUInfo.ackCode = DFU_ACK_ERR_FLSH;
			goto DFU_START_EXIT;;
		}

	}else if (QCOM_OTA_OK != qcom_ota_partition_erase())
	{
		DBG_STR("qcom_ota_partition_erase : fail....\r\n");
		gDFUInfo.ackCode = DFU_ACK_ERR_FLSH;
		goto DFU_START_EXIT;;
	}

	gDFUInfo.stat = DFU_STAT_START;

	//flashIdx = DFU_HDPKT_LEN;
	gDFUInfo.idx = 0;
	gDFUInfo.offset = 0;

	return ;

DFU_START_EXIT:

	qcom_ota_session_end(0);
}

static void dfu_going(u8 *lpDats, u16 len)
{
	u32 n;

	if (gDFUInfo.stat != DFU_STAT_START && gDFUInfo.stat != DFU_STAT_GOING)
	{
		gDFUInfo.ackCode = DFU_ACK_ERR_STAT;
		return;
	}

	if (!gDFUInfo.offset)
	{
		while (len)
		{
			if (qcom_ota_parse_image_hdr((A_UINT8 * )lpDats, (A_UINT32*)&gDFUInfo.offset) == QCOM_OTA_OK)
			{
				gDFUInfo.idx = 0;
				len -= gDFUInfo.offset;
				lpDats += gDFUInfo.offset;
				break;
			}
			lpDats++;
			len--;
		}
		//DBG_TRACE("dfu_going...OFFSET(%d)\r\n",gDFUInfo.offset);
		//return;
	}


	while (len > 0)
	{
		if (qcom_ota_partition_write_data(gDFUInfo.idx,
				(A_UINT8 * )lpDats, len, (A_UINT32*)&n) != QCOM_OTA_OK)
		{
			//SWAT_PTF("Write Fail\r\n");
			gDFUInfo.ackCode = DFU_ACK_ERR_FLSH;
			break;
		}

		lpDats += n;
		gDFUInfo.idx += n;
		if (len > n)
			len -= n;
		else
			break;
	}

	gDFUInfo.stat = DFU_STAT_GOING;
}

static void dfu_verify(u8 *lpDats, u16 len)
{
	u8 f = 0;

	if (gDFUInfo.stat != DFU_STAT_GOING)
	{
		DBG_STR("dfu_verify...fail2\r\n");
		gDFUInfo.ackCode = DFU_ACK_ERR_STAT;
		goto VERIFY_EXIT;
	}

	if (qcom_ota_partition_verify_checksum() != QCOM_OTA_OK)
	{
		DBG_STR("dfu_verify...fail3\r\n");
		gDFUInfo.ackCode = DFU_ACK_ERR_SUM;
		goto VERIFY_EXIT;
	}

	f = 1;

VERIFY_EXIT:

	if (QCOM_OTA_COMPLETED != qcom_ota_session_end(f))
	{
		gDFUInfo.ackCode = DFU_ACK_UPGRADE_FAID;
		DBG_STR("dfu_verify...fail4\r\n");
	}

	gDFUInfo.stat = DFU_STAT_VERFY;
}

u8 dfu_proc(u8 cmd, u8 *lpDats, u16 len)
{
	gDFUInfo.ackCode = DFU_ACK_OK;

	switch (cmd)
	{
	case CTRL_DFU_START:
		dfu_stat_start(lpDats, len);
		break;
	case CTRL_DFU_GOING:
		dfu_going(lpDats, len);
		break;
	case CTRL_DFU_FINISH:
		dfu_verify(lpDats, len);
		break;
	case CTRL_DFU_CANCEL:
		dfu_Init();
		break;
	default:
		break;
	}

	if (gDFUInfo.ackCode & 0x80)
	{
		gDFUInfo.stat = DFU_STAT_IDLE;
	}

	return gDFUInfo.ackCode;
}

