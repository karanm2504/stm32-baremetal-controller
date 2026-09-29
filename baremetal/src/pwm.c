#include "stm32c0xx.h"
#include "pwm.h"

void pwm_init(void)
{
    /* Enable GPIOA clock */
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    /* PA6 = alternate-function mode */
    GPIOA->MODER &= ~(3U << 12);
    GPIOA->MODER |=  (2U << 12);

    /* PA6 alternate function = AF1 for TIM3_CH1 */
    GPIOA->AFR[0] &= ~(0xFU << 24);
    GPIOA->AFR[0] |=  (1U << 24);

    /* Enable TIM3 peripheral clock */
    RCC->APBENR1 |= RCC_APBENR1_TIM3EN;

    /*
     * 48 MHz / (47 + 1) = 1 MHz timer counter
     * 1 MHz / (999 + 1) = 1 kHz PWM frequency
     */
    TIM3->PSC = 47U;
    TIM3->ARR = 999U;

    /* Start with 0% duty cycle */
    TIM3->CCR1 = 0U;

    /* TIM3 channel 1 = PWM mode 1 */
    TIM3->CCMR1 &= ~TIM_CCMR1_OC1M_Msk;
    TIM3->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos);

    /* Enable CCR1 preload */
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE;

    /* Enable channel 1 output */
    TIM3->CCER |= TIM_CCER_CC1E;

    /* Enable ARR preload */
    TIM3->CR1 |= TIM_CR1_ARPE;

    /* Load the prescaler, ARR and CCR values */
    TIM3->EGR |= TIM_EGR_UG;

    /* Start TIM3 */
    TIM3->CR1 |= TIM_CR1_CEN;
}

void pwm_set_duty(uint16_t duty)
{
    /* Limit the input to the valid 0–1000 range */
    if (duty > 1000U)
    {
        duty = 1000U;
    }

    TIM3->CCR1 = duty;
}