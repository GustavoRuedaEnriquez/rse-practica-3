#include <stdint.h>

#ifndef LIN_H_
#define LIN_H_

typedef struct {
   uint8_t ID;
   uint8_t * (*Clbk)(void);
} SlaveIdHandler;

static uint8_t g_master_state = 4;

static uint8_t g_slave_state  = 0;

void lin_init_uart(SlaveIdHandler *table, uint8_t slaves_number);

uint8_t lin_start_master(uint8_t id);

// Master stuff
void lin_sm_master();
void lin_state_wait_until_next_frame();
void lin_state_send_break();
void lin_state_send_synch();
void lin_send_protected_identifier(uint8_t id);

// Slave stuff
void lin_sm_slave();
void lin_state_wait_header();
void lin_state_slave_handler();
void lin_send_response(uint8_t length);

#endif /* LIN_H_ */
