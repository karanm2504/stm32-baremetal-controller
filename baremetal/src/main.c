#include "stm32c0xx.h"
#include "uart.h"
#include "adc.h"
#include "pwm.h"
#include "spi.h"
#include "i2c.h"
#include <stdint.h>
#include <string.h>
#define MPU6050_ADDR       0x68U
#define MPU6050_WHO_AM_I   0x75U


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
    char rx_buffer[32];
    uint8_t rx_index = 0U;
    uint16_t sensor_value = 0U;
	uint8_t who_am_i = 0U;
    uint8_t spi_received = 0U;
    /* FIRST: Enable GPIOA clock */
    RCC->IOPENR |= (1U << 0); 


//spi
/* Enable GPIOA clock */
RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

/* Initialize USART2 on PA2 and PA3 */
uart_init();



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
    i2c_init(); 
    spi1_init();
    uart_write_string("Hello from STM32\r\n");
    spi_received = spi1_loopback_test(0xA5U);
    uart_write_string("SPI sent: 165\r\n");
    uart_write_string("SPI received: ");
    uart_write_uint16(spi_received);
    uart_write_string("\r\n");


if (i2c_read_register(MPU6050_ADDR,
                      MPU6050_WHO_AM_I,
                      &who_am_i))
{
    uart_write_string("MPU6050 ACK\r\n");

    if (who_am_i == 0x68U)
    {
        uart_write_string("WHO_AM_I correct: 0x68\r\n");
    }
 else
{
    uart_write_string("WHO_AM_I incorrect. Received value = ");
    uart_write_uint16(who_am_i);
    uart_write_string("\r\n");
}
}
else
{
    uart_write_string("WHO_AM_I failed at stage: ");
    uart_write_uint16(i2c_get_stage());
    uart_write_string("\r\n");
}


    while (1)
{

sensor_value = adc_read();

uint32_t pwm_value =
    ((uint32_t)sensor_value * 1000U) / 4095U;

uint32_t pwm_percent =
    (pwm_value * 100U) / 1000U;

    pwm_set_duty((uint16_t)pwm_value);
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
