/*
  * Copyright (c) 2015 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */
#ifndef _WIFI_H_
#define _WIFI_H_
#include "app_common.h"
#include "qcom/qcom_internal.h"

#define TCP_CFGPARAM_IDX 	0
#define TCP_DATPARAM_IDX	1

//-------sys_mode------------------
#define CONFIG_MODE     1
#define OTA_MODE        2

#define UNCONNT	0
#define CONNTING	1
#define CONNTED	2
typedef struct  _SYS_STAT_INFO
{
  uint8_t sys_mode;//SP OR STA
  uint8_t state;
  uint8_t WifiConnected;
  uint8_t UdpConnected;
  uint8_t UsbDeviceState;
  uint8_t DataSocketConnected;
  uint8_t ConfigSocketConnected;
}SYS_STAT_INFO;
extern SYS_STAT_INFO g_state_data;
#endif







