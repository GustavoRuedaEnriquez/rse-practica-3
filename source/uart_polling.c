/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LIN.h"

#include "board.h"
#include "fsl_uart.h"

#include "pin_mux.h"
#include "clock_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define NUM_SLAVES 3

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
uint8_t * callback_1();
uint8_t * callback_2();
uint8_t * callback_3();

/*******************************************************************************
 * Variables
 ******************************************************************************/


uint8_t txbuff[]   = "Eart polling example\r\nBoard will send back received characters\r\n";
uint8_t rxbuff[20];

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Main function
 */
int main(void)
{

    SlaveIdHandler slaves_table[NUM_SLAVES] = {
    		{0x23,callback_1},
			{0x24,callback_2},
			{0x25,callback_3}
    };

    lin_init_uart(slaves_table, NUM_SLAVES);


    UART_WriteBlocking(UART0, txbuff, sizeof(txbuff) - 1);

    int i = 0;
    while(txbuff[i] != '\0') {
    	UART_WriteBlocking(UART1, &txbuff[i], 1);
    	UART_ReadBlocking(UART2, &rxbuff[i], 1);
    	i++;
    }



    while (1)
    {
//    	sm_master(g_state);
    	//UART_ReadBlocking(DEMO_UART, &ch, 1);
        //UART_WriteBlocking(DEMO_UART, &ch, 1);
    }
}

uint8_t * callback_1()
{
	static uint8_t r[] = "C1";
	return r;
}

uint8_t * callback_2(){
	static uint8_t r[] = "C2";
	return r;
}

uint8_t * callback_3(){
	static uint8_t r[] = "C3";
	return r;
}
