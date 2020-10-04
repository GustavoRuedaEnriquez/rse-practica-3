/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board.h"
#include "fsl_uart.h"

#include "pin_mux.h"
#include "clock_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* UART instance and clock */
#define DEMO_UART          UART0
#define DEMO_UART_CLKSRC   UART0_CLK_SRC
#define DEMO_UART_CLK_FREQ CLOCK_GetFreq(UART0_CLK_SRC)


/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void init_uart();

void sm_slave(uint8_t state);

// Master stuff
void sm_master(uint8_t state);
void state_wait_until_next_frame();
void state_send_break();
void state_send_synch();
void send_protected_identifier(uint8_t id, uint8_t len);





/*******************************************************************************
 * Variables
 ******************************************************************************/
uint8_t g_state = 2;

uint8_t txbuff[]   = "Uart polling example\r\nBoard will send back received characters\r\n";
uint8_t rxbuff[20] = {0};

/*******************************************************************************
 * Code
 ******************************************************************************/
void sm_master(uint8_t state)
{
	switch(state)
	{
	case 1 :
		state_wait_until_next_frame();
		break;
	case 2 :
		state_send_break();
		g_state++;
		break;
	case 3 :
		state_send_synch();
		g_state++;
		break;
	case 4 :
		send_protected_identifier(0x8, 0x2);
		g_state = 2;
		break;
	}
}

void state_wait_until_next_frame()
{
	while(1)
	{

	}
}

void state_send_break()
{
	uint8_t synch_break[] = {0, 0x80};
	UART_WriteBlocking(DEMO_UART, synch_break, sizeof(synch_break) - 1);
	UART_WriteBlocking(DEMO_UART, synch_break, sizeof(synch_break) - 1);
}

void state_send_synch()
{
	uint8_t synch = 0x55;
	UART_WriteBlocking(DEMO_UART, &synch, sizeof(synch) - 1);
}

void send_protected_identifier(uint8_t id, uint8_t len)
{
	uint8_t identifier = id | (len << 4);
	uint8_t p0 = (identifier && 0x1) ^ ((identifier>>1) && 0x1) ^ ((identifier>>2) && 0x1) ^ ((identifier>>4) && 0x1);
	uint8_t p1 = ~(((identifier>>1) && 0x1) ^ ((identifier>>3) && 0x1) ^ ((identifier>>4) && 0x1) ^ ((identifier>>5) && 0x1));
	identifier = (p0 << 6) | (p1<<7) | identifier;
	UART_WriteBlocking(DEMO_UART, &identifier, sizeof(identifier) - 1);
}

/*!
 * @brief Main function
 */

void init_uart()
{
	uart_config_t config;

	BOARD_InitPins();
	BOARD_BootClockRUN();

	/*
	 * config.baudRate_Bps = 115200U;
	 * config.parityMode = kUART_ParityDisabled;
	 * config.stopBitCount = kUART_OneStopBit;
	 * config.txFifoWatermark = 0;
	 * config.rxFifoWatermark = 1;
	 * config.enableTx = false;
	 * config.enableRx = false;
	 */
	UART_GetDefaultConfig(&config);
	config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
	config.enableTx     = true;
	config.enableRx     = true;

	UART_Init(DEMO_UART, &config, DEMO_UART_CLK_FREQ);
}

int main(void)
{
    uint8_t ch;

    init_uart();

    //UART_WriteBlocking(DEMO_UART, txbuff, sizeof(txbuff) - 1);

    while (1)
    {
    	sm_master(g_state);
    	//UART_ReadBlocking(DEMO_UART, &ch, 1);
        //UART_WriteBlocking(DEMO_UART, &ch, 1);
    }
}
