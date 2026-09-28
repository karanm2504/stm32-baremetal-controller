#ifndef SPI_H
#define SPI_H

#include <stdint.h>

void spi1_init(void);
uint8_t spi1_transfer(uint8_t data);
uint8_t spi1_loopback_test(uint8_t data);

#endif