#include "board.h"
#include "fsl_uart.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "LIN.h"

/* UART instance and clock */
#define TERM_UART UART0
#define TERM_UART_CLKSRC   UART0_CLK_SRC
#define TERM_UART_CLK_FREQ CLOCK_GetFreq(UART0_CLK_SRC)

#define LIN_UART  UART1
#define LIN_UART_CLKSRC   UART1_CLK_SRC
#define LIN_UART_CLK_FREQ CLOCK_GetFreq(UART1_CLK_SRC)

uint8_t g_channel;
uint8_t g_master_state = 1;
uint8_t g_slave_state  = 0;

uint8_t g_master_len = 0;
uint8_t g_slave_len  = 0;

uint8_t g_sample_data[] = "JA";

uint8_t *g_result = 0;

uint8_t g_identifier_passed = 0;

SlaveIdHandler *g_slaves_table;

uint8_t g_slaves_number = 0;

void init_uart(SlaveIdHandler *table, uint8_t slaves_number)
{
	uart_config_t config;

	BOARD_InitPins();
	BOARD_BootClockRUN();

	UART_GetDefaultConfig(&config);
	config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
	config.enableTx     = true;
	config.enableRx     = true;

	UART_Init(LIN_UART, &config, TERM_UART_CLK_FREQ);

	g_slaves_table = table;
	g_slaves_number = slaves_number;
}

// MASTER
void sm_master(uint8_t state)
{
	switch(state)
	{
	case 0 :
		lin_state_wait_until_next_frame();
		g_master_state++;
		break;
	case 1 :
		lin_state_send_break();
		g_master_state++;
		break;
	case 2 :
		lin_state_send_synch();
		g_master_state++;
		break;
	case 3 :
		g_master_len = 0x2;
		lin_send_protected_identifier(0x8, g_master_len);
		g_master_state = 0;
		break;
	}
}

void lin_state_wait_until_next_frame()
{
	uint8_t rxbuff[g_master_len];
	for(uint8_t i = 0; i < g_master_len; i++)
	{
		rxbuff[i] = UART_ReadBlocking(LIN_UART, &g_channel, 1);
		UART_WriteBlocking(TERM_UART, rxbuff, sizeof(rxbuff) - 1);
	}
}

void lin_state_send_break()
{
	uint8_t synch_break[] = {0, 0x80};
	UART_WriteBlocking(LIN_UART, synch_break, sizeof(synch_break) - 1);
	UART_WriteBlocking(LIN_UART, synch_break, sizeof(synch_break) - 1);
}

void lin_state_send_synch()
{
	uint8_t synch = 0x55;
	UART_WriteBlocking(LIN_UART, &synch, sizeof(synch) - 1);
}

void lin_send_protected_identifier(uint8_t id, uint8_t len)
{
	uint8_t identifier = id | (len << 4);
	uint8_t p0 = (identifier && 0x1) ^ ((identifier>>1) && 0x1) ^ ((identifier>>2) && 0x1) ^ ((identifier>>4) && 0x1);
	uint8_t p1 = ~(((identifier>>1) && 0x1) ^ ((identifier>>3) && 0x1) ^ ((identifier>>4) && 0x1) ^ ((identifier>>5) && 0x1));
	identifier = (p0 << 6) | (p1<<7) | identifier;
	UART_WriteBlocking(LIN_UART, &identifier, sizeof(identifier) - 1);
}


// SLAVE
void lin_sm_slave(uint8_t state)
{
	switch(state)
	{
		case 0 :
			lin_state_wait_header();
			g_slave_state++;
			break;
		case 1 :
			lin_stat_slave_handler();
			break;
		case 2 :
			lin_send_response(g_slave_len);
			g_slave_state = 0;
			break;
		}
}

void lin_state_wait_header()
{
	uint8_t rxbuff[3] = {0};
	for(uint8_t i = 0; i < 3; i++)
	{
		rxbuff[i] = UART_ReadBlocking(LIN_UART, &g_channel, 1);
		UART_WriteBlocking(TERM_UART, rxbuff, sizeof(rxbuff) - 1);
	}
	g_identifier_passed = rxbuff[2];
}

void lin_state_slave_handler()
{
	uint8_t error[] = "Error en la paridad";

	uint8_t id = g_identifier_passed & 0xF;
	g_slave_len = ( g_identifier_passed >> 4 ) & 0x3;

	uint8_t p0 = (g_identifier_passed && 0x1) ^ ((g_identifier_passed>>1) && 0x1) ^ ((g_identifier_passed>>2) && 0x1) ^ ((g_identifier_passed>>4) && 0x1);
	uint8_t p1 = ~(((g_identifier_passed>>1) && 0x1) ^ ((g_identifier_passed>>3) && 0x1) ^ ((g_identifier_passed>>4) && 0x1) ^ ((g_identifier_passed>>5) && 0x1));

	if(( p0 == (g_identifier_passed >> 6) & 0x1) && (p1 == g_identifier_passed >> 7) )
	{
		for(uint8_t i = 0; i < g_slaves_number; i++)
		{
			if(id == g_slaves_table[i].ID)
			{
				g_result = g_slaves_table[i].Clbk();
				g_slave_state++;
			}
		}
	} else {
		UART_WriteBlocking(TERM_UART, error, sizeof(error) - 1);
		g_slave_state = 0;
	}
}

void lin_send_response(uint8_t length)
{

	UART_WriteBlocking(LIN_UART, g_result, length - 1);

	uint8_t checksum = 0;

	for(uint8_t i = 0; i < length; i++)
	{
		checksum += g_result[i];
	}

	checksum = ~(checksum);

	UART_WriteBlocking(LIN_UART, checksum, sizeof(checksum) - 1);
}

uint8_t * dummy_callback(uint8_t length)
{
	return g_sample_data;
}
