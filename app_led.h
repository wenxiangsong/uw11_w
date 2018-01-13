/*
 * app_led.h
 *
 *  Created on: Jan 7, 2018
 *      Author: root
 */

#ifndef APP_LED_H_
#define APP_LED_H_
#include "base.h"
#include "basetypes.h"
#include "devDbg.h"
#include "qcom_gpio.h"
#define LED_NUMBER  2
#define led_unflash   0
#define led_isflash   1
#define led_flashfinsh 2
typedef struct _LED_STATE_INFO
{
	A_UINT32 PinNumber[LED_NUMBER];
	A_UINT32 flashstate[LED_NUMBER];
}LED_STATE_INFO;
extern LED_STATE_INFO led;
static inline void app_led_on(int PinNum)
{
	if(qcom_gpio_pin_set(PinNum,TRUE) != A_OK)
	{
		DBG_TRACE("set led pin %d level error\r\n", PinNum);
	}
}
static inline void app_led_off(int PinNum)
{
	if (qcom_gpio_pin_set(PinNum,FALSE) != A_OK)
	{
		DBG_TRACE("set led pin %d level error\r\n", PinNum);
	}
}
static inline void app_led_flash_on(int PinNum)
{
	int i;
	for(i=0;i<LED_NUMBER;i++)
	{
		if(led.PinNumber[i]==PinNum)
		{
			if(led.flashstate[i]==led_unflash)
			{
				led.flashstate[i]=led_isflash;
			}
			break;
		}
	}
}
static inline void app_led_flash_off(int PinNum)
{
	int i;
	for (i = 0; i < LED_NUMBER; i++)
	{
		if (led.PinNumber[i]==PinNum)
		{
			if(led.flashstate[i]!=led_unflash)
			{
				led.flashstate[i] = led_unflash;
			}
			break;
		}
	}
}
#endif /* APP_LED_H_ */
