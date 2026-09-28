#include "stm32c0xx.h"
#include "adc.h"

void adc_init(void)
{
    /* Enable GPIOA and ADC clocks */
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    RCC->APBENR2 |= RCC_APBENR2_ADCEN;

    /* PA1 = analog mode (11) */
    GPIOA->MODER &= ~(3U << 2);
    GPIOA->MODER |=  (3U << 2);

    /* ADC conversion clock = PCLK / 2 */
    ADC1->CFGR2 &= ~ADC_CFGR2_CKMODE_Msk;
    ADC1->CFGR2 |= (1U << ADC_CFGR2_CKMODE_Pos);

    /* Enable ADC internal voltage regulator */
    ADC1->CR |= ADC_CR_ADVREGEN;

    /* Allow the regulator to start */
    for (volatile uint32_t i = 0U; i < 1000U; i++)
    {
    }

    /* Calibrate the ADC */
    ADC1->CR |= ADC_CR_ADCAL;

    while (ADC1->CR & ADC_CR_ADCAL)
    {
    }

    /* Clear the old ADC-ready flag */
    ADC1->ISR = ADC_ISR_ADRDY;

    /* Enable ADC */
    ADC1->CR |= ADC_CR_ADEN;

    /* Wait until ADC is ready */
    while (!(ADC1->ISR & ADC_ISR_ADRDY))
    {
    }

    /* Clear the channel-configuration-ready flag */
    ADC1->ISR = ADC_ISR_CCRDY;

    /* Select PA1 = ADC channel 1 */
    ADC1->CHSELR = ADC_CHSELR_CHSEL1;

    /* Wait until channel selection is applied */
    while (!(ADC1->ISR & ADC_ISR_CCRDY))
    {
    }
}

uint16_t adc_read(void)
{
    /* Start one ADC conversion */
    ADC1->CR |= ADC_CR_ADSTART;

    /* Wait until conversion finishes */
    while (!(ADC1->ISR & ADC_ISR_EOC))
    {
    }

    /* Return the 12-bit value: 0 to 4095 */
    return (uint16_t)ADC1->DR;
}