#ifndef I2C_H
#define I2C_H

#include <stdint.h>

void i2c_init(void);

uint8_t i2c_read_register(uint8_t device_address,
                          uint8_t register_address,
                          uint8_t *value);

uint8_t i2c_get_stage(void);

#endif