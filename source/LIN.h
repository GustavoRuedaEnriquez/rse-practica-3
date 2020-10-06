#include <stdint.h>

#ifndef LIN_H_
#define LIN_H_

typedef struct {
   uint8_t ID;
   uint8_t (*Clbk)(void);
} SlaveIdHandler;

void lin_init_uart();

// Master stuff
void lin_sm_master(uint8_t state);
void lin_state_wait_until_next_frame();
void lin_state_send_break();
void lin_state_send_synch();
void lin_send_protected_identifier(uint8_t id, uint8_t len);

// Slave stuff
void lin_sm_slave(uint8_t state);
void lin_state_wait_header();
void lin_state_slave_handler();
void lin_send_response(uint8_t length);

uint8_t * dummy_callback(uint8_t length);
#endif /* LIN_H_ */
