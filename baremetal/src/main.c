#include "stm32c0xx.h"
#include <stdint.h>

volatile uint8_t button_event = 0U;

void EXTI0_1_IRQHandler(void)
{
    if (EXTI->RPR1 & (1U << 0))
    {
        EXTI->RPR1 = (1U << 0);
        button_event = 1U;
    }
}

int main(void)
{
    uint8_t led_on = 0U;

    /* Enable GPIOA clock */
    RCC->IOPENR |= (1U << 0);

    /* PA5 = output */
    GPIOA->MODER &= ~(3U << 10);
    GPIOA->MODER |=  (1U << 10);

    /* PA0 = input */
    GPIOA->MODER &= ~(3U << 0);

    /* PA0 internal pull-down */
    GPIOA->PUPDR &= ~(3U << 0);
    GPIOA->PUPDR |=  (2U << 0);

    /* EXTI0 source = Port A */
    EXTI->EXTICR[0] &= ~(0xFFU << 0);

    /* Rising edge enabled */
    EXTI->RTSR1 |= (1U << 0);

    /* Falling edge disabled */
    EXTI->FTSR1 &= ~(1U << 0);

    /* Clear any old rising pending flag */
    EXTI->RPR1 = (1U << 0);

    /* Enable EXTI line 0 interrupt */
    EXTI->IMR1 |= (1U << 0);

    /* Enable EXTI0/1 interrupt in NVIC */
    NVIC_EnableIRQ(EXTI0_1_IRQn);

    while (1)
    {
        if (button_event == 1U)
        {
            button_event = 0U;

            led_on ^= 1U;

            if (led_on == 1U)
            {
                GPIOA->BSRR = (1U << 5);
            }
            else
            {
                GPIOA->BSRR = (1U << 21);
            }
        }
    }
}
