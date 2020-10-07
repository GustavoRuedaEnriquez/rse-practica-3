#include "board.h"
#include "fsl_uart.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "LIN.h"

/* UART instance and clock */
#define TERM_UART          UART0
#define TERM_UART_CLKSRC   UART0_CLK_SRC
#define TERM_UART_CLK_FREQ CLOCK_GetFreq(UART0_CLK_SRC)

#define LIN_UART_MASTER          UART1
#define LIN_UART_MASTER_CLKSRC   UART1_CLK_SRC
#define LIN_UART_MASTER_CLK_FREQ CLOCK_GetFreq(UART1_CLK_SRC)

#define LIN_UART_SLAVE          UART2
#define LIN_UART_SLAVE_CLKSRC   UART2_CLK_SRC
#define LIN_UART_SLAVE_CLK_FREQ CLOCK_GetFreq(UART2_CLK_SRC)

uint8_t g_master_len = 0;

uint8_t g_slave_len  = 0;

uint8_t g_flag = 0;

uint8_t *g_result = 0;

uint8_t g_identifier_passed = 0;

SlaveIdHandler *g_slaves_table;

uint8_t g_slaves_number = 0;

uint8_t g_id = 0;

void lin_init_uart(SlaveIdHandler *table, uint8_t slaves_number)
{
	uart_config_t config;

	BOARD_InitPins();
	BOARD_BootClockRUN();

	UART_GetDefaultConfig(&config);
	config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
	config.enableTx     = true;
	config.enableRx     = true;

	UART_Init(LIN_UART_MASTER, &config, LIN_UART_MASTER_CLK_FREQ);
	UART_Init(LIN_UART_SLAVE, &config, LIN_UART_SLAVE_CLK_FREQ);
	UART_Init(TERM_UART, &config, TERM_UART_CLK_FREQ);

	g_slaves_table = table;
	g_slaves_number = slaves_number;
}

// Master
void lin_sm_master()
{
	switch(g_master_state)
	{
	case 0 :
		lin_state_wait_until_next_frame();
		break;
	case 1 :
		lin_state_send_break();
		break;
	case 2 :
		lin_state_send_synch();
		g_master_state++;
		break;
	case 3 :
		lin_send_protected_identifier(g_id);
		g_master_state = 0;
		break;
	case 4 :
		break;
	}
}

uint8_t lin_start_master(uint8_t id) {
	if(g_master_state == 4) {
		g_master_state = 1;
		uint8_t msg[] = "\r\nInicio de envio de datos\r\n";
		g_id = id;
		UART_WriteBlocking(TERM_UART, msg, sizeof(msg) - 1);
		return 0;
	}
	return -1;

}

void lin_state_wait_until_next_frame()
{
	static uint8_t counter = 0;
	static uint8_t rx_buff[8];

	if(g_flag == 1) {
		UART_ReadBlocking(LIN_UART_MASTER, &rx_buff[counter], 1);
		UART_WriteBlocking(TERM_UART, &rx_buff[counter], 1);
		if(counter < g_master_len) {
			counter++;
		} else {
			uint8_t msg[] = "\r\nEnvio exitoso\r\n";
			uint8_t checksum = 0;
			for(uint8_t i = 0; i < g_master_len + 1; i++)
			{
				checksum += rx_buff[i];
			}
			if(checksum == 0xFF) {
				UART_WriteBlocking(TERM_UART, msg, sizeof(msg) - 1);
			}
			g_master_state = 4;
			g_flag = 0;
			counter = 0;
		}
	}
}

void lin_state_send_break()
{
	static uint8_t counter = 0;
	uint8_t synch_break[] = {0x00, 0x80};
	g_flag = 1;
	UART_WriteBlocking(LIN_UART_MASTER, &synch_break[counter], 1);
	if(counter < 1) {
		counter++;
	} else {
		counter = 0;
		g_master_state++;
	}

}

void lin_state_send_synch()
{
	uint8_t synch = 0x55;
	UART_WriteBlocking(LIN_UART_MASTER, &synch, 1);
}

void lin_send_protected_identifier(uint8_t id)
{
	uint8_t identifier = id;
	g_master_len = identifier >> 4;
	switch(g_master_len) {
	case 0:
			g_master_len = 2;
			break;
	case 1:
			g_master_len = 2;
			break;
	case 2:
			g_master_len = 4;
			break;
	case 3:
			g_master_len = 8;
			break;
	}

	uint8_t p0 = (identifier && 0x1) ^ ((identifier>>1) && 0x1) ^ ((identifier>>2) && 0x1) ^ ((identifier>>4) && 0x1);
	uint8_t p1 = ~(((identifier>>1) && 0x1) ^ ((identifier>>3) && 0x1) ^ ((identifier>>4) && 0x1) ^ ((identifier>>5) && 0x1));
	identifier = (p0 << 6) | (p1<<7) | identifier;
	UART_WriteBlocking(LIN_UART_MASTER, &identifier, 1);
}


// Slave
void lin_sm_slave()
{
	switch(g_slave_state)
	{
		case 0 :
			lin_state_wait_header();
			break;
		case 1 :
			lin_state_slave_handler();
			break;
		case 2 :
			lin_send_response(g_slave_len);
			break;
		}
}

void lin_state_wait_header()
{
	static uint8_t counter = 0;
	static uint8_t rx_buff[4];

	if(g_flag == 1) {
		UART_ReadBlocking(LIN_UART_SLAVE, &rx_buff[counter], 1);
		UART_WriteBlocking(TERM_UART, &rx_buff[counter], 1);
		if(counter < 3) {
			counter++;
		} else {
			g_identifier_passed = rx_buff[3];
			g_slave_state++;
			g_flag = 0;
			counter = 0;
		}
	}
}

void lin_state_slave_handler()
{
	uint8_t error[] = "\r\nError en la paridad\r\n";
	uint8_t error_id[] = "\r\nID no existe\r\n";
	uint8_t id = g_identifier_passed & 0x3F;
	g_slave_len = ( id >> 4 ) & 0x3;

	switch(g_slave_len) {
	case 0:
			g_slave_len = 2;
			break;
	case 1:
			g_slave_len = 2;
			break;
	case 2:
			g_slave_len = 4;
			break;
	case 3:
			g_slave_len = 8;
			break;
	}

	uint8_t p0 = (g_identifier_passed && 0x1) ^ ((g_identifier_passed>>1) && 0x1) ^ ((g_identifier_passed>>2) && 0x1) ^ ((g_identifier_passed>>4) && 0x1);
	uint8_t p1 = ~(((g_identifier_passed>>1) && 0x1) ^ ((g_identifier_passed>>3) && 0x1) ^ ((g_identifier_passed>>4) && 0x1) ^ ((g_identifier_passed>>5) && 0x1));

	p0 &= 0x1;
	p1 &= 0x1;

	if( ( p0 == ((g_identifier_passed >> 6) & 0x1)) && (p1 == g_identifier_passed >> 7) )
	{
		for(uint8_t i = 0; i < g_slaves_number; i++)
		{
			if(id == g_slaves_table[i].ID)
			{
				if(NULL != g_slaves_table[i].Clbk) {
					g_result = g_slaves_table[i].Clbk();
					g_slave_state++;
				}
			}
		}

		if(g_slave_state == 0x1) {
			UART_WriteBlocking(TERM_UART, error_id, sizeof(error_id) - 1);
			g_slave_state = 0;
		}

	} else {
		UART_WriteBlocking(TERM_UART, error, sizeof(error) - 1);
		g_slave_state = 0;
	}
}

void lin_send_response(uint8_t length)
{
	static uint8_t counter = 0;
	g_flag = 1;
	if(counter < length) {
		UART_WriteBlocking(LIN_UART_SLAVE, &g_result[counter], 1);
		counter++;
	} else {
		uint8_t checksum = 0;
		for(uint8_t i = 0; i < length; i++)
		{
			checksum += g_result[i];
		}
		checksum = ~(checksum);
		UART_WriteBlocking(LIN_UART_SLAVE, &checksum, 1);
		counter = 0;
		g_slave_state = 0;
	}
}
