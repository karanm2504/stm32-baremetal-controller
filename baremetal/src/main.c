#include "stm32c0xx.h"
#include "uart.h"
#include "adc.h"
#include <stdint.h>
#include <string.h>
#define MPU6050_ADDR       0x68U
#define MPU6050_WHO_AM_I   0x75U
volatile uint8_t i2c_stage = 0U;

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

//i2c
void i2c_gpio_init(void)
{
    /* Enable GPIO Port B clock */
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
    (void)RCC->IOPENR;  /* Allow clock enable to take effect */

    /* Open-drain: pins can pull LOW or release the line */
    GPIOB->OTYPER |= (1U << 8) | (1U << 9);

    /* No internal pulls: external 4.7k resistors are fitted */
    GPIOB->PUPDR &= ~((3U << 16) | (3U << 18));

    /* PB8 and PB9 use AF6 for I2C1 */
    GPIOB->AFR[1] &= ~((0xFU << 0) | (0xFU << 4));
    GPIOB->AFR[1] |=  ((6U << 0) | (6U << 4));

    /* PB8 and PB9 = alternate-function mode */
    GPIOB->MODER &= ~((3U << 16) | (3U << 18));
    GPIOB->MODER |=  ((2U << 16) | (2U << 18));


}

void i2c_init(void)
{


    /* Enable clock access to I2C1 */
    RCC->APBENR1 |= RCC_APBENR1_I2C1EN;
    (void)RCC->APBENR1;

    /* Keep I2C disabled while configuring it */
    I2C1->CR1 &= ~I2C_CR1_PE;

    /* Select SYSCLK as the I2C1 clock source */
    RCC->CCIPR &= ~RCC_CCIPR_I2C1SEL;
    RCC->CCIPR |= RCC_CCIPR_I2C1SEL_0;
/* 3. Reset I2C1 peripheral */
    RCC->APBRSTR1 |=  (1U << 21);
    RCC->APBRSTR1 &= ~(1U << 21);

    /* 4. Enable I2C1 clock */
    RCC->APBENR1 |= (1U << 21);
	 I2C1->TIMINGR = 0x20303E5DU;
    I2C1->CR1 |= I2C_CR1_PE;


}


uint8_t i2c_read_register(uint8_t device_address,
                          uint8_t register_address,
                          uint8_t *value)
{
    uint32_t timeout = 100000U;

    /* Clear old I2C flags */
    I2C1->ICR = I2C_ICR_NACKCF |
                I2C_ICR_STOPCF |
                I2C_ICR_BERRCF |
                I2C_ICR_ARLOCF;

uart_write_string("Register argument = ");
uart_write_uint16(register_address);
uart_write_string("\r\n");



    /*
     * Stage 1:
     * Send the device address in WRITE mode.
     * AUTOEND automatically generates STOP after one byte.
     */
 
/* Stage 1: preload the register address BEFORE START */
/*
 * Stage 1:
 * Preload register address, then start write without AUTOEND.
 */
i2c_stage = 1U;

/* Load WHO_AM_I register address */
I2C1->TXDR = register_address;

/* One-byte write, SOFTEND: do not generate STOP */
I2C1->CR2 =
      ((uint32_t)device_address << 1)
    | (1U << I2C_CR2_NBYTES_Pos)
    | I2C_CR2_START;

/*
 * Stage 2:
 * Wait until register-address write is complete.
 */
i2c_stage = 2U;
timeout = 100000U;

while ((I2C1->ISR &
       (I2C_ISR_TC |
        I2C_ISR_NACKF |
        I2C_ISR_BERR |
        I2C_ISR_ARLO)) == 0U)
{
    if (--timeout == 0U)
    {
        uart_write_string("Stage 2 timeout, ISR = ");
        uart_write_uint16((uint16_t)I2C1->ISR);
        uart_write_string("\r\n");
        return 0U;
    }
}

if (I2C1->ISR &
   (I2C_ISR_NACKF |
    I2C_ISR_BERR |
    I2C_ISR_ARLO))
{
    uart_write_string("Stage 2 error\r\n");
    return 0U;
}
    /*
     * Stage 3:
     * Start a new transaction in READ mode.
     */
    i2c_stage = 3U;
    timeout = 100000U;

    I2C1->CR2 =
          ((uint32_t)device_address << 1)
        | (1U << I2C_CR2_NBYTES_Pos)
        | I2C_CR2_RD_WRN
        | I2C_CR2_AUTOEND
        | I2C_CR2_START;

    while ((I2C1->ISR &
           (I2C_ISR_RXNE |
            I2C_ISR_NACKF |
            I2C_ISR_BERR |
            I2C_ISR_ARLO)) == 0U)
    {
        if (--timeout == 0U)
        {
            uart_write_string("Stage 3 timeout, ISR = ");
            uart_write_uint16((uint16_t)I2C1->ISR);
            uart_write_string("\r\n");
            return 0U;
        }
    }

    if (I2C1->ISR &
       (I2C_ISR_NACKF |
        I2C_ISR_BERR |
        I2C_ISR_ARLO))
    {
        uart_write_string("Stage 3 error, ISR = ");
        uart_write_uint16((uint16_t)I2C1->ISR);
        uart_write_string("\r\n");
        return 0U;
    }

    /* Read WHO_AM_I value */
    *value = (uint8_t)I2C1->RXDR;

    /*
     * Stage 4:
     * Wait for the read transaction's automatic STOP.
     */
    i2c_stage = 4U;
    timeout = 100000U;

    while ((I2C1->ISR & I2C_ISR_STOPF) == 0U)
    {
        if (--timeout == 0U)
        {
            uart_write_string("Stage 4 timeout\r\n");
            return 0U;
        }
    }

    I2C1->ICR = I2C_ICR_STOPCF;

    i2c_stage = 5U;
    return 1U;
}




/* SPI*/
void spi1_gpio_init(void)
{
    /* Enable GPIOB clock */
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;

    /*
     * PB3 = SCK
     * PB4 = MISO
     * PB5 = MOSI
     * Set all three to alternate-function mode: 10
     */
    GPIOB->MODER &= ~((3U << 6) |
                      (3U << 8) |
                      (3U << 10));

    GPIOB->MODER |=  ((2U << 6) |
                      (2U << 8) |
                      (2U << 10));

    /* PB3, PB4 and PB5 use AF0 for SPI1 */
    GPIOB->AFR[0] &= ~((0xFU << 12) |
                       (0xFU << 16) |
                       (0xFU << 20));

    /* PB0 = normal GPIO output for CS */
    GPIOB->MODER &= ~(3U << 0);
    GPIOB->MODER |=  (1U << 0);

    /* CS starts HIGH: device inactive */
    GPIOB->BSRR = (1U << 0);
}



void spi1_init(void)
{
    /* Enable SPI1 peripheral clock */
    RCC->APBENR2 |= RCC_APBENR2_SPI1EN;

    /* Keep SPI disabled while configuring */
    SPI1->CR1 &= ~SPI_CR1_SPE;

    /*
     * Controller/master mode
     * Software-controlled CS
     * Internal NSS kept HIGH
     * Clock = 48 MHz / 8 = 6 MHz
     */
    SPI1->CR1 = SPI_CR1_MSTR |
                SPI_CR1_SSM  |
                SPI_CR1_SSI  |
                SPI_CR1_BR_1;

    /*
     * 8-bit data frame:
     * DS = 0111 means 8 bits
     *
     * RXNE becomes active after receiving 8 bits
     */
    SPI1->CR2 = (7U << SPI_CR2_DS_Pos) |
                SPI_CR2_FRXTH;

    /* Enable SPI1 */
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t spi1_transfer(uint8_t transmit_data)
{
    /* Wait until the transmit register is empty */
    while ((SPI1->SR & SPI_SR_TXE) == 0U)
    {
    }

    /* Send one 8-bit byte through MOSI */
    *((volatile uint8_t *)&SPI1->DR) = transmit_data;

    /* Wait until one byte arrives through MISO */
    while ((SPI1->SR & SPI_SR_RXNE) == 0U)
    {
    }

    /* Read and return the received byte */
    return *((volatile uint8_t *)&SPI1->DR);
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
	i2c_gpio_init();
i2c_init();
spi1_gpio_init();
spi1_init();



uart_write_string("Hello from STM32\r\n");

/* Select the SPI device: CS LOW */
GPIOB->BSRR = (1U << 16);

/* Send 0xA5 and receive the looped-back byte */
spi_received = spi1_transfer(0xA5U);

/* Wait until the final clock pulse finishes */
while (SPI1->SR & SPI_SR_BSY)
{
}

/* End the transfer: CS HIGH */
GPIOB->BSRR = (1U << 0);

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
    uart_write_uint16(i2c_stage);
    uart_write_string("\r\n");
}










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
