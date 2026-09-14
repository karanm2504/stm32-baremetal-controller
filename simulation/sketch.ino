#include "stm32c0xx.h"

void setup()
{
    RCC->IOPENR |= (1U << 0);

    GPIOA->MODER &= ~(3U << 10);
    GPIOA->MODER |=  (1U << 10);
}

void loop()
{
    GPIOA->BSRR = (1U << 5);
    delay(1000);

    GPIOA->BSRR = (1U << 21);
    delay(1000);
}
