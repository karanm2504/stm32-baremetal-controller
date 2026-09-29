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

i2c_status_t i2c_read_register(uint8_t device_address,
                               uint8_t register_address,
                               uint8_t *value)
{
    uint32_t timeout;
    uint32_t status;

    /* Check pointer */
    if (value == 0)
    {
        return I2C_STATUS_INVALID_PARAM;
    }

    /* Clear old flags */
    I2C1->ICR = I2C_ICR_NACKCF |
                I2C_ICR_STOPCF |
                I2C_ICR_BERRCF |
                I2C_ICR_ARLOCF;

    /*
     * Stage 1:
     * Wait until the bus is free.
     */
    i2c_stage = 1U;
    timeout = 100000U;

    while ((I2C1->ISR & I2C_ISR_BUSY) != 0U)
    {
        if (--timeout == 0U)
        {
            uart_write_string("Stage 1: bus busy timeout\r\n");
            return I2C_STATUS_BUSY_TIMEOUT;
        }
    }

    /*
     * Stage 2:
     * Send register address.
     */
    i2c_stage = 2U;

    I2C1->CR2 =
          ((uint32_t)device_address << 1)
        | (1U << I2C_CR2_NBYTES_Pos)
        | I2C_CR2_AUTOEND
        | I2C_CR2_START;

    I2C1->TXDR = register_address;

    timeout = 100000U;

    while ((I2C1->ISR &
           (I2C_ISR_STOPF |
            I2C_ISR_NACKF |
            I2C_ISR_BERR |
            I2C_ISR_ARLO)) == 0U)
    {
        if (--timeout == 0U)
        {
            uart_write_string("Stage 2 TX timeout, ISR = ");
            uart_write_uint16((uint16_t)I2C1->ISR);
            uart_write_string("\r\n");

            return I2C_STATUS_TX_TIMEOUT;
        }
    }

    status = I2C1->ISR;

    if ((status & I2C_ISR_NACKF) != 0U)
    {
        I2C1->ICR = I2C_ICR_NACKCF;

        uart_write_string("Stage 2: NACK\r\n");

        return I2C_STATUS_NACK;
    }

    if ((status & I2C_ISR_BERR) != 0U)
    {
        I2C1->ICR = I2C_ICR_BERRCF;

        uart_write_string("Stage 2: bus error\r\n");

        return I2C_STATUS_BUS_ERROR;
    }

    if ((status & I2C_ISR_ARLO) != 0U)
    {
        I2C1->ICR = I2C_ICR_ARLOCF;

        uart_write_string("Stage 2: arbitration lost\r\n");

        return I2C_STATUS_ARBITRATION_LOST;
    }

    /* Clear STOP from write transaction */
    I2C1->ICR = I2C_ICR_STOPCF;

    /*
     * Stage 3:
     * Wait until bus becomes free again.
     */
    i2c_stage = 3U;
    timeout = 100000U;

    while ((I2C1->ISR & I2C_ISR_BUSY) != 0U)
    {
        if (--timeout == 0U)
        {
            uart_write_string("Stage 3: bus busy timeout\r\n");

            return I2C_STATUS_BUSY_TIMEOUT;
        }
    }

    /*
     * Stage 4:
     * Read one byte.
     */
    i2c_stage = 4U;

    I2C1->CR2 =
          ((uint32_t)device_address << 1)
        | (1U << I2C_CR2_NBYTES_Pos)
        | I2C_CR2_RD_WRN
        | I2C_CR2_AUTOEND
        | I2C_CR2_START;

    timeout = 100000U;

    while ((I2C1->ISR &
           (I2C_ISR_RXNE |
            I2C_ISR_NACKF |
            I2C_ISR_BERR |
            I2C_ISR_ARLO)) == 0U)
    {
        if (--timeout == 0U)
        {
            uart_write_string("Stage 4 RX timeout, ISR = ");
            uart_write_uint16((uint16_t)I2C1->ISR);
            uart_write_string("\r\n");

            return I2C_STATUS_RX_TIMEOUT;
        }
    }

    status = I2C1->ISR;

    if ((status & I2C_ISR_NACKF) != 0U)
    {
        I2C1->ICR = I2C_ICR_NACKCF;

        uart_write_string("Stage 4: NACK\r\n");

        return I2C_STATUS_NACK;
    }

    if ((status & I2C_ISR_BERR) != 0U)
    {
        I2C1->ICR = I2C_ICR_BERRCF;

        uart_write_string("Stage 4: bus error\r\n");

        return I2C_STATUS_BUS_ERROR;
    }

    if ((status & I2C_ISR_ARLO) != 0U)
    {
        I2C1->ICR = I2C_ICR_ARLOCF;

        uart_write_string("Stage 4: arbitration lost\r\n");

        return I2C_STATUS_ARBITRATION_LOST;
    }

    *value = (uint8_t)I2C1->RXDR;

    /*
     * Stage 5:
     * Wait for STOP after read.
     */
    i2c_stage = 5U;
    timeout = 100000U;

    while ((I2C1->ISR & I2C_ISR_STOPF) == 0U)
    {
        if (--timeout == 0U)
        {
            uart_write_string("Stage 5: STOP timeout\r\n");

            return I2C_STATUS_RX_TIMEOUT;
        }
    }

    I2C1->ICR = I2C_ICR_STOPCF;

    i2c_stage = 6U;

    return I2C_STATUS_OK;
}

uint8_t i2c_get_stage(void)
{
    return i2c_stage;
}