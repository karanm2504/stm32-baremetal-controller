#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

void gpio_init(void);
void led_set(uint8_t on);
uint8_t button_is_pressed(void);
uint8_t button_event_take(void);

#endif  