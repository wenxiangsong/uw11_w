/*
 * app_gpio.c

 *
 *  Created on: Jan 7, 2018
 *      Author: root
 */
#include "app_gpio.h"
#include "app_main.h"
#include <qcom/qcom_ir_ctrl.h>
#include "app_def_sys_err.h"
#include "devDbg.h"
static A_UINT32 dev_gpio_input[]={DEV_HID_STAT_INFO_PIN,DEV_USB_STAT_INFO_PIN,DEV_SPI_RTS_PIN,DEV_REST_FACTORY_PIN};
static A_UINT32 dev_gpio_output[]={LED_CONTROL_PROT_PIN,LED_DATA_PORT_PIN,LED_UDP_PORT_PIN,LED_WIFI_PIN,LED_UART_PIN,WIFI_STAT_INFO_PIN,SEND_REQ_PIN};
static A_UINT32 app_gpio_init(int PinNum,A_BOOL InPut,A_BOOL HighLevle)
{
    A_INT32 retVal;
    A_UINT32 Peripheral_Id=QCOM_PERIPHERAL_ID_GPIOn(PinNum);
    DBG_TRACE("Peripheral_Id %8x\r\n",Peripheral_Id);
    if(InPut==FALSE)
    {
    	gpio_pin_peripheral_config_t    configs;
    	configs.peripheral_id=QCOM_PERIPHERAL_ID_GPIO;
    	configs.gpio_active_config=GPIO_ACTIVE_VALUE;
    	if(qcom_gpio_add_alternate_configurations(PinNum,1, &configs)!=SYS_OK)
    		retVal=SYS_ERR24;
    	else if(qcom_gpio_apply_peripheral_configuration(Peripheral_Id, FALSE)!= SYS_OK)
    		retVal=SYS_ERR25;
    }
    if(qcom_gpio_pin_source(PinNum, FALSE)!=SYS_OK)
    	retVal=SYS_ERR26;
    else if(qcom_gpio_pin_dir(PinNum, InPut) != SYS_OK)
    	retVal=SYS_ERR27;
    else if(qcom_gpio_pin_set(PinNum,HighLevle) != SYS_OK)
    	retVal=SYS_ERR28;
    else
    	retVal=SYS_OK;
    DBG_TRACE("gpio init %d error(%d)\r\n",PinNum,retVal);
    return retVal;
}
static A_UINT32 app_gpio_input_init(void)
{
	A_UINT32 retVal,i;
	for(i=0;i<(sizeof(dev_gpio_input)/sizeof(A_UINT32));i++)
	{
		retVal=app_gpio_init(dev_gpio_input[i],TRUE,FALSE);
		if(retVal!=SYS_OK)
			break;
	}
	return retVal;
}
static A_UINT32 app_gpio_output_init(void)
{
	A_UINT32 retVal,i;
	for(i=0;i<(sizeof(dev_gpio_output)/sizeof(A_UINT32));i++)
	{
		retVal=app_gpio_init(dev_gpio_output[i],FALSE,FALSE);
		if(retVal!=SYS_OK)
			break;
	}
	return retVal;
}
A_UINT32 dev_sys_gpio_init()
{
	A_UINT32 retVal;
	retVal=app_gpio_input_init();
	if(retVal==SYS_OK)
		return app_gpio_output_init();
	else
		return retVal;
}




