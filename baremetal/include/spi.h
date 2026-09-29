#ifndef SPI_H
#define SPI_H

#include <stdint.h>

typedef enum
{
    SPI_STATUS_OK = 0,
    SPI_STATUS_TX_TIMEOUT,
    SPI_STATUS_RX_TIMEOUT,
    SPI_STATUS_BUSY_TIMEOUT,
    SPI_STATUS_LOOPBACK_MISMATCH,
    SPI_STATUS_INVALID_PARAM
} spi_status_t;

void spi1_init(void);

spi_status_t spi1_transfer(
    uint8_t tx_data,
    uint8_t *rx_data
);

spi_status_t spi1_loopback_test(uint8_t data);

#endif