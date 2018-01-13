/*
 * app_gpio.h
 *
 *  Created on: Jan 7, 2018
 *      Author: root
 */

#ifndef APP_GPIO_H_
#define APP_GPIO_H_
#include "qcom_common.h"
#include <qcom/qcom_gpio.h>
#define GPIO_ACTIVE_VALUE		0x80000008
/*******IN_PUT_PIN***********************/
#define DEV_HID_STAT_INFO_PIN	7
#define DEV_USB_STAT_INFO_PIN	8
#define DEV_SPI_RTS_PIN			13
#define DEV_REST_FACTORY_PIN	31
/*******OUT_PUT_PIN**********************/
#define LED_CONTROL_PROT_PIN	2
#define LED_DATA_PORT_PIN		3
#define LED_UDP_PORT_PIN		6
#define LED_WIFI_PIN			10
#define LED_UART_PIN			11
#define WIFI_STAT_INFO_PIN		12
#define SEND_REQ_PIN			14
/*******SPI_CONTROL_PIN******************/
#define DEV_SPI_SEL_PIN			0
#define DEV_SPI_MOSI_PIN		1
#define DEV_SPI_MISO_PIN		4
#define DEV_SPI_CLK_PIN			5
A_UINT32 dev_sys_gpio_init();
#endif /* APP_GPIO_H_ */
