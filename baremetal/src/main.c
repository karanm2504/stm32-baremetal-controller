#include "stm32c0xx.h"
#include <stdint.h>
#include <string.h>

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

void uart_write_char(char c)
{
    while (!(USART2->ISR & USART_ISR_TXE_TXFNF))
    {
    }

    USART2->TDR = (uint8_t)c;
}

void uart_write_string(const char *str)
{
    while (*str != '\0')
    {
        uart_write_char(*str);
        str++;
    }
}

char uart_read_char(void)
{
    while (!(USART2->ISR & USART_ISR_RXNE_RXFNE))
    {
    }

    return (char)USART2->RDR;
}

/*22sep ADC*/
uint16_t adc_read(void)
{
    /* Start one conversion */
    ADC1->CR |= ADC_CR_ADSTART;

    /* Wait until conversion is complete */
    while (!(ADC1->ISR & ADC_ISR_EOC))
    {
    }

    /* Return 12-bit ADC result */
    return (uint16_t)ADC1->DR;
}


void adc_init(void)
{
    /* Enable ADC peripheral clock */
    RCC->APBENR2 |= RCC_APBENR2_ADCEN;
	/* ADC conversion clock = PCLK / 2 */
	ADC1->CFGR2 &= ~ADC_CFGR2_CKMODE_Msk;
	ADC1->CFGR2 |=  (1U << ADC_CFGR2_CKMODE_Pos);

    /* Enable ADC internal regulator */
    ADC1->CR |= ADC_CR_ADVREGEN;

    /* Small startup delay */
    for (volatile uint32_t i = 0U; i < 1000U; i++)
    {
    }

    /* Calibrate ADC */
    ADC1->CR |= ADC_CR_ADCAL;

    while (ADC1->CR & ADC_CR_ADCAL)
    {
    }

    /* Clear old ADC-ready flag */
    ADC1->ISR = ADC_ISR_ADRDY;

    /* Enable ADC */
    ADC1->CR |= ADC_CR_ADEN;

    /* Wait until ADC is ready */
    while (!(ADC1->ISR & ADC_ISR_ADRDY))
    {
    }

    /* Clear old channel-config-ready flag */
    ADC1->ISR = ADC_ISR_CCRDY;

    /* Select PA1 = ADC channel 1 */
    ADC1->CHSELR = ADC_CHSELR_CHSEL1;

    /* Wait until channel selection is applied */
    while (!(ADC1->ISR & ADC_ISR_CCRDY))
    {
    }
}

 
/*potentiometer*/
void uart_write_uint16(uint16_t value)
{
    char buffer[6];
    uint8_t index = 0U;

    if (value == 0U)
    {
        uart_write_char('0');
        return;
    }

    while (value > 0U)
    {
        buffer[index] = (char)('0' + (value % 10U));
        value /= 10U;
        index++;
    }

    while (index > 0U)
    {
        index--;
        uart_write_char(buffer[index]);
    }
}



/*PWM*/
	void pwm_init(void)
{
    /* 1. Enable GPIOA clock */
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    /* 2. PA6 = Alternate Function mode */
    GPIOA->MODER &= ~(3U << 12);
    GPIOA->MODER |=  (2U << 12);

    /* 3. PA6 alternate function = AF1 */
    GPIOA->AFR[0] &= ~(0xFU << 24);
    GPIOA->AFR[0] |=  (1U << 24);

    /* 4. Enable TIM3 clock */
    RCC->APBENR1 |= RCC_APBENR1_TIM3EN;

    /* 5. Timer timing */
    TIM3->PSC  = 47U;      // 48 MHz / 48 = 1 MHz
    TIM3->ARR  = 999U;     // 1 MHz / 1000 = 1 kHz PWM
    TIM3->CCR1 = 250;     // 50% duty cycle

    /* 6. TIM3 Channel 1 = PWM mode 1 */
    TIM3->CCMR1 &= ~TIM_CCMR1_OC1M_Msk;
    TIM3->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos);

    /* 7. Enable CCR1 preload */
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE;

    /* 8. Enable Channel 1 output */
    TIM3->CCER |= TIM_CCER_CC1E;

    /* 9. Enable ARR preload */
    TIM3->CR1 |= TIM_CR1_ARPE;

    /* 10. Load PSC/ARR/CCR values */
    TIM3->EGR |= TIM_EGR_UG;

    /* 11. Start timer */
    TIM3->CR1 |= TIM_CR1_CEN;
}





int main(void)
{

    uint8_t led_on = 0U;
    uint8_t debounce_active = 0U;
    uint32_t debounce_start = 0U;
    char rx_buffer[32];
    uint8_t rx_index = 0U;
    uint16_t sensor_value = 0U;

    /* FIRST: Enable GPIOA clock */
    RCC->IOPENR |= (1U << 0); 




/* PA2 = alternate function */
GPIOA->MODER &= ~(3U << 4);
GPIOA->MODER |=  (2U << 4);

/* PA2 = AF1 = USART2_TX */
GPIOA->AFR[0] &= ~(0xFU << 8);
GPIOA->AFR[0] |=  (1U << 8);

/* Enable USART2 clock */
RCC->APBENR1 |= RCC_APBENR1_USART2EN;

/* 115200 baud */
USART2->BRR = (SystemCoreClock + 57600U) / 115200U;

/* Enable transmitter and USART */
USART2->CR1 |= USART_CR1_TE | USART_CR1_UE;
/* PA3 = alternate function */
GPIOA->MODER &= ~(3U << 6);
GPIOA->MODER |=  (2U << 6);

/* PA3 = AF1 = USART2_RX */
GPIOA->AFR[0] &= ~(0xFU << 12);
GPIOA->AFR[0] |=  (1U << 12);

/* Enable TX + RX + USART */
USART2->CR1 |= USART_CR1_TE |
               USART_CR1_RE |
               USART_CR1_UE;
/* PA1 = analog mode ---ADC */
GPIOA->MODER &= ~(3U << 2);
GPIOA->MODER |=  (3U << 2);



    /* ----------------------------------------
       GPIO CONFIGURATION
       ---------------------------------------- */

   

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

	
	 /* ADC setup */
    adc_init();
	pwm_init();

	uart_write_string("Hello from STM32\r\n");






    while (1)
{

sensor_value = adc_read();

uint32_t pwm_value =
    ((uint32_t)sensor_value * 1000U) / 4095U;

uint32_t pwm_percent =
    (pwm_value * 100U) / 1000U;

TIM3->CCR1 = pwm_value;
    if (USART2->ISR & USART_ISR_RXNE_RXFNE)
    {
        char c = (char)USART2->RDR;

        uart_write_char(c);

        if ((c == '\r') || (c == '\n'))
        {
            rx_buffer[rx_index] = '\0';

            if (rx_index > 0U)
            {
                if (strcmp(rx_buffer, "help") == 0)
                {
                    uart_write_string("\r\nCommands: help, status, led on, led off\r\n");
                }
                else if (strcmp(rx_buffer, "status") == 0)
                {
		uart_write_string("\r\nSystem running\r\n"); 
		uart_write_string("Sensor: ");
    		uart_write_uint16(sensor_value);
    		uart_write_string("\r\n");  
		uart_write_string("PWM Duty: ");
	uart_write_uint16((uint16_t)pwm_percent);
	uart_write_string("%\r\n");
			/* Convert ADC value to millivolts */
    uint32_t voltage_mv = ((uint32_t)sensor_value * 3300U) / 4095U;

    /* Print voltage */
    uart_write_string("Voltage: ");
    uart_write_uint16((uint16_t)voltage_mv);
    uart_write_string(" mV\r\n");

    /* Print sensor state */

		uart_write_string("Sensor State: ");

	if (sensor_value < 1500U)
	{
    	uart_write_string("LOW\r\n");
	}
	else if (sensor_value < 3000U)
	{
    	uart_write_string("NORMAL\r\n");
	}
	else
	{
    	uart_write_string("HIGH\r\n");
	}	
           }
                else if (strcmp(rx_buffer, "led on") == 0)
                {
                    led_on = 1U;
                    GPIOA->BSRR = (1U << 5);
                    uart_write_string("\r\nLED ON\r\n");
                }
                else if (strcmp(rx_buffer, "led off") == 0)
                {
                    led_on = 0U;
                    GPIOA->BSRR = (1U << 21);
                    uart_write_string("\r\nLED OFF\r\n");
                }
                else
                {
                    uart_write_string("\r\nUnknown command\r\n");
                }
            }

            rx_index = 0U;
        }
        else
        {
            if (rx_index < 31U)
            {
                rx_buffer[rx_index] = c;
                rx_index++;
            }
        }
    }

    /* keep your existing button/debounce logic here too */




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
