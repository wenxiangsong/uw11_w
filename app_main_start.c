/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

//#include "qcom_system.h"
#include "qcom_common.h"
#include "threadx/tx_api.h"
#include "qcom_cli.h"
#include "threadxdmn_api.h"
#include "qcom_wps.h"
#include "qcom_gpio.h"
#define BYTE_POOL_SIZE (4*1024 + 512 )
#define PSEUDO_HOST_STACK_SIZE (4* 1024)   /* small stack for pseudo-Host thread */
TX_THREAD host_thread;
TX_BYTE_POOL pool;
void
main_task_entry(ULONG which_thread)
{
    extern void user_pre_init(void);
    extern void main_task(void);
    user_pre_init();
    A_PRINTF("app_main_task start...\r\n");
    main_task();
}
void user_main(void)
{   
    tx_byte_pool_create(&pool, "cdrtest pool", TX_POOL_CREATE_DYNAMIC, BYTE_POOL_SIZE);

    {
        CHAR *pointer;
        tx_byte_allocate(&pool, (VOID **) & pointer, PSEUDO_HOST_STACK_SIZE, TX_NO_WAIT);

        tx_thread_create(&host_thread, "cdrtest thread", main_task_entry,
                         0, pointer, PSEUDO_HOST_STACK_SIZE, 16, 16, 4, TX_AUTO_START);
    }
    cdr_threadx_thread_init();
}



