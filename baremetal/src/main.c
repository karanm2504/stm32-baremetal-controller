#include "stm32c0xx.h"

int main(void)
{
    /* Enable GPIOA clock */
    RCC->IOPENR |= (1U << 0);

    /* PA5 = output for LED */
    GPIOA->MODER &= ~(3U << 10);
    GPIOA->MODER |=  (1U << 10);

    /* PA0 = input for button */
    GPIOA->MODER &= ~(3U << 0);

    /* PA0 = internal pull-down */
    GPIOA->PUPDR &= ~(3U << 0);
    GPIOA->PUPDR |=  (2U << 0);

    while (1)
    {
        if (GPIOA->IDR & (1U << 0))
        {
            GPIOA->BSRR = (1U << 5);
        }
        else
        {
            GPIOA->BSRR = (1U << 21);
        }
    }
}
