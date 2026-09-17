#include "stm32c0xx.h"
#include <stdint.h>

/* Software event flag set by the button ISR */
volatile uint8_t button_event = 0U;

/* Millisecond counter updated by SysTick */
volatile uint32_t system_ms = 0U;


/* Runs automatically every 1 ms */
void SysTick_Handler(void)
{
    system_ms++;
}


/* Runs automatically when EXTI line 0/1 interrupt occurs */
void EXTI0_1_IRQHandler(void)
{
    /* Check whether EXTI line 0 caused the interrupt */
    if (EXTI->RPR1 & (1U << 0))
    {
        /* Clear rising-edge pending flag */
        EXTI->RPR1 = (1U << 0);

        /* Tell main() that a button event happened */
        button_event = 1U;
    }
}


int main(void)
{
    uint8_t led_on = 0U;
uint8_t debounce_active = 0U;
uint32_t debounce_start = 0U;


    /* ----------------------------------------
       GPIO CONFIGURATION
       ---------------------------------------- */

    /* Enable GPIOA peripheral clock */
    RCC->IOPENR |= (1U << 0);


    /* PA5 = output for onboard LED */

    /* Clear PA5 MODER bits 11:10 */
    GPIOA->MODER &= ~(3U << 10);

    /* Set PA5 MODER bits to 01 = output */
    GPIOA->MODER |= (1U << 10);


    /* Start with LED OFF */
    GPIOA->BSRR = (1U << 21);


    /* PA0 = input for push button */

    /* PA0 MODER bits 1:0 = 00 = input */
    GPIOA->MODER &= ~(3U << 0);


    /* PA0 internal pull-down */

    /* Clear PA0 PUPDR bits */
    GPIOA->PUPDR &= ~(3U << 0);

    /* 10 = pull-down */
    GPIOA->PUPDR |= (2U << 0);



    /* ----------------------------------------
       EXTI CONFIGURATION
       ---------------------------------------- */

    /* Connect EXTI line 0 to GPIO Port A */
    EXTI->EXTICR[0] &= ~(0xFFU << 0);


    /* Enable rising-edge trigger for EXTI0 */
    EXTI->RTSR1 |= (1U << 0);


    /* Disable falling-edge trigger */
    EXTI->FTSR1 &= ~(1U << 0);


    /* Clear any old pending rising-edge event */
    EXTI->RPR1 = (1U << 0);


    /* Unmask / enable EXTI line 0 interrupt */
    EXTI->IMR1 |= (1U << 0);


    /* Enable EXTI0/EXTI1 interrupt in NVIC */
    NVIC_EnableIRQ(EXTI0_1_IRQn);



    /* ----------------------------------------
       SYSTICK CONFIGURATION
       ---------------------------------------- */

    /*
       Configure SysTick for 1000 interrupts/sec.

       1000 interrupts/sec
       = one interrupt every 1 ms
    */
    SysTick_Config(SystemCoreClock / 1000U);



    /* ----------------------------------------
       MAIN SUPERLOOP
       ---------------------------------------- */


    	while (1)
{
    /* Did EXTI report a possible button press? */
    if (button_event == 1U)
    {
        button_event = 0U;

        /* Start debounce only if we are not already debouncing */
        if (debounce_active == 0U)
        {
            debounce_active = 1U;
            debounce_start = system_ms;
        }
    }

    /* Are we currently waiting for the button signal to settle? */
    if (debounce_active == 1U)
    {
        /* Has 30 ms passed? */
        if ((system_ms - debounce_start) >= 30U)
        {
            debounce_active = 0U;

            /* Check whether PA0 is STILL HIGH */
            if (GPIOA->IDR & (1U << 0))
            {
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
}
}
