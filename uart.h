#ifndef UART_H
#define UART_H

#ifdef __cplusplus
extern "C" {
#endif
  
#include <stdint.h>

void uart_init(int id, uint32_t baud);
uint32_t uart_send(int id, uint8_t* buffer, uint32_t len);
uint32_t uart_read(int id, uint8_t* buffer, uint32_t len);
void uart1_irqhandler(void);
uint32_t uart_getrxlen(int id);

#ifdef __cplusplus
}
#endif

#endif