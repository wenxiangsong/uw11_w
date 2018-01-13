/*

 * app_led.c
 *
 *  Created on: Jan 7, 2018
 *      Author: root
 */
#include "app_led.h"
#include "app_gpio.h"
LED_STATE_INFO led;
qcom_timer_t led_led_timer;
static int dev_led_pin[LED_NUMBER]={10,11};
static void app_led_state_set(int PinNum,A_BOOL on)
{
	 if (qcom_gpio_pin_set(PinNum,on) != A_OK)
	{
		DBG_TRACE("set led pin %d level error\r\n", PinNum);
	}else
	{
		DBG_TRACE("set led pin %d level  %d OK\r\n", PinNum,on);
	}
}
static void app_led_flash_handler(unsigned int alarm, void *arg)
{
	int i;
	for(i=0;i<LED_NUMBER;i++)
	{
		switch(led.flashstate[i])
		{
			case led_unflash:
				break;
			case led_isflash:
				app_led_state_set((int)led.PinNumber[i], !qcom_gpio_pin_get((int)led.PinNumber[i]));
				led.flashstate[i]=led_flashfinsh;
				break;
			case led_flashfinsh:
				app_led_state_set((int)led.PinNumber[i], !qcom_gpio_pin_get((int)led.PinNumber[i]));
				led.flashstate[i]=led_unflash;
				break;
			default:
			break;
		}
	}
}
void app_dev_led_init(void)
{
	int i = 0;
	for (i = 0; i < LED_NUMBER; i++)
	{
		led.PinNumber[i]=dev_led_pin[i];
		led.flashstate[i]=led_isflash;
		app_led_flash_off(10);
		app_led_flash_off(11);
	}
	qcom_timer_init(&led_led_timer, app_led_flash_handler, NULL, 500,PERIODIC);
	qcom_timer_start(&led_led_timer);
}





