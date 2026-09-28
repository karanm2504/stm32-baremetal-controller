#include "stm32c0xx.h"
#include "i2c.h"
#include "uart.h"

static volatile uint8_t i2c_stage = 0U;

static void i2c_gpio_init(void)
{
    /* Enable GPIOB clock */
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
    (void)RCC->IOPENR;

    /* PB8 = SCL and PB9 = SDA: open-drain outputs */
    GPIOB->OTYPER |= (1U << 8) | (1U << 9);

    /* No internal pulls because external 4.7 kΩ pull-ups are used */
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
    i2c_gpio_init();

    /* Enable I2C1 peripheral clock */
    RCC->APBENR1 |= RCC_APBENR1_I2C1EN;
    (void)RCC->APBENR1;

    /* Keep I2C disabled during configuration */
    I2C1->CR1 &= ~I2C_CR1_PE;

    /* Select SYSCLK as the I2C1 clock source */
    RCC->CCIPR &= ~RCC_CCIPR_I2C1SEL;
    RCC->CCIPR |= RCC_CCIPR_I2C1SEL_0;

    /* Reset and release the I2C1 peripheral */
    RCC->APBRSTR1 |=  (1U << 21);
    RCC->APBRSTR1 &= ~(1U << 21);

    /* Re-enable the I2C1 clock after reset */
    RCC->APBENR1 |= RCC_APBENR1_I2C1EN;

    /* Timing value used for the current 48 MHz clock setup */
    I2C1->TIMINGR = 0x20303E5DU;

    /* Enable I2C1 */
    I2C1->CR1 |= I2C_CR1_PE;
}

uint8_t i2c_read_register(uint8_t device_address,
                          uint8_t register_address,
                          uint8_t *value)
{
    uint32_t timeout = 100000U;

    /* Clear old error and STOP flags */
    I2C1->ICR = I2C_ICR_NACKCF |
                I2C_ICR_STOPCF |
                I2C_ICR_BERRCF |
                I2C_ICR_ARLOCF;

    uart_write_string("Register argument = ");
    uart_write_uint16(register_address);
    uart_write_string("\r\n");

    /*
     * Stage 1:
     * Send the register address in write mode.
     */
    i2c_stage = 1U;

    I2C1->TXDR = register_address;

    I2C1->CR2 =
          ((uint32_t)device_address << 1)
        | (1U << I2C_CR2_NBYTES_Pos)
        | I2C_CR2_START;

    /*
     * Stage 2:
     * Wait until the register-address write completes.
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
     * Generate a repeated START and read one byte.
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

    /* Read the received register value */
    *value = (uint8_t)I2C1->RXDR;

    /*
     * Stage 4:
     * Wait for the automatic STOP condition.
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

uint8_t i2c_get_stage(void)
{
    return i2c_stage;
}