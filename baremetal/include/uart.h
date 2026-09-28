#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_write_char(char c);
void uart_write_string(const char *str);
char uart_read_char(void);
void uart_write_uint16(uint16_t value);

#endif