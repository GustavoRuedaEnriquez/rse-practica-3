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
uint8_t txbuff[]   = "\r\nPractica 3 - LIN\r\n";
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

    uint8_t  i = 0;
    while (1)
    {
    	if(lin_start_master(slaves_table[i].ID) == 0) {
    	//if(lin_start_master(0x38) == 0) {
    		i = (i + 1) % NUM_SLAVES;
    	}
    	lin_sm_master(g_master_state);
    	lin_sm_slave(g_slave_state);
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
