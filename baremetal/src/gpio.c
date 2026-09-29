#include "stm32c0xx.h"
#include "gpio.h"

static volatile uint8_t button_event = 0U;

void EXTI0_1_IRQHandler(void)
{
    /* Check whether EXTI line 0 caused the interrupt */
    if (EXTI->RPR1 & (1U << 0))
    {
        /* Clear the rising-edge pending flag */
        EXTI->RPR1 = (1U << 0);

        /* Inform the main program about the button event */
        button_event = 1U;
    }
}

void gpio_init(void)
{
    /* Enable GPIOA clock */
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    /* PA5 = output for the onboard LED */
    GPIOA->MODER &= ~(3U << 10);
    GPIOA->MODER |=  (1U << 10);

    /* Start with the LED off */
    GPIOA->BSRR = (1U << 21);

    /* PA0 = input for the push button */
    GPIOA->MODER &= ~(3U << 0);

    /* PA0 internal pull-down */
    GPIOA->PUPDR &= ~(3U << 0);
    GPIOA->PUPDR |=  (2U << 0);

    /* Connect EXTI line 0 to GPIO Port A */
    EXTI->EXTICR[0] &= ~(0xFFU << 0);

    /* Enable rising-edge trigger */
    EXTI->RTSR1 |= (1U << 0);

    /* Disable falling-edge trigger */
    EXTI->FTSR1 &= ~(1U << 0);

    /* Clear any old pending event */
    EXTI->RPR1 = (1U << 0);

    /* Unmask EXTI line 0 */
    EXTI->IMR1 |= (1U << 0);

    /* Enable EXTI0/EXTI1 interrupt in the NVIC */
    NVIC_EnableIRQ(EXTI0_1_IRQn);
}

void led_set(uint8_t on)
{
    if (on != 0U)
    {
        /* Set PA5 HIGH */
        GPIOA->BSRR = (1U << 5);
    }
    else
    {
        /* Reset PA5 LOW */
        GPIOA->BSRR = (1U << 21);
    }
}

uint8_t button_is_pressed(void)
{
    if (GPIOA->IDR & (1U << 0))
    {
        return 1U;
    }

    return 0U;
}

uint8_t button_event_take(void)
{
    uint8_t event = button_event;
    button_event = 0U;
    return event;
}