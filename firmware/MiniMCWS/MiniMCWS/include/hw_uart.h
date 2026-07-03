/*
 * hw_uart.h
 *
 * Created: 2026-07-02 오후 6:03:19
 *  Author: shldl
 */ 


#ifndef HW_UART_H_
#define HW_UART_H_

#include <stdint.h>
#include <stdbool.h>

void uart_init(void);
void uart_putc(uint8_t c);
void uart_puts(const char* str);
void uart_send_packet(uint8_t mode, int16_t temp, uint16_t light, uint8_t error_code, uint8_t seq);
void uart_send_telemetry(uint8_t mode, int16_t temp, uint16_t light, uint8_t error_code);
bool uart_get_packet_status(void); 
void uart_clear_packet_status(void);
uint32_t uart_get_last_heartbeat_tick(void);
uint8_t uart_get_injected_faults(void);

#endif /* HW_UART_H_ */