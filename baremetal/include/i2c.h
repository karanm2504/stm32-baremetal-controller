#ifndef I2C_H
#define I2C_H

#include <stdint.h>

typedef enum
{
    I2C_STATUS_OK = 0,
    I2C_STATUS_BUSY_TIMEOUT,
    I2C_STATUS_TX_TIMEOUT,
    I2C_STATUS_RX_TIMEOUT,
    I2C_STATUS_NACK,
    I2C_STATUS_BUS_ERROR,
    I2C_STATUS_ARBITRATION_LOST,
    I2C_STATUS_INVALID_PARAM
} i2c_status_t;

void i2c_init(void);

i2c_status_t i2c_read_register(
    uint8_t device_address,
    uint8_t register_address,
    uint8_t *data
);

#endif