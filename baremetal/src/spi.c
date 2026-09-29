#include "stm32c0xx.h"
#include "spi.h"

#include <stdint.h>

#define SPI_TIMEOUT 100000U

static void spi1_gpio_init(void)
{
    /*
     * Enable GPIOB clock.
     */
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
    (void)RCC->IOPENR;

    /*
     * PB3 = SCK
     * PB4 = MISO
     * PB5 = MOSI
     *
     * Configure all three as alternate-function pins.
     */
    GPIOB->MODER &= ~(
        (3U << 6)  |
        (3U << 8)  |
        (3U << 10)
    );

    GPIOB->MODER |= (
        (2U << 6)  |
        (2U << 8)  |
        (2U << 10)
    );

    /*
     * PB3, PB4 and PB5 use AF0 for SPI1.
     */
    GPIOB->AFR[0] &= ~(
        (0xFU << 12) |
        (0xFU << 16) |
        (0xFU << 20)
    );

    /*
     * PB0 = normal GPIO output for software chip select.
     */
    GPIOB->MODER &= ~(3U << 0);
    GPIOB->MODER |=  (1U << 0);

    /*
     * CS starts HIGH:
     * peripheral inactive.
     */
    GPIOB->BSRR = (1U << 0);
}

void spi1_init(void)
{
    spi1_gpio_init();

    /*
     * Enable SPI1 peripheral clock.
     */
    RCC->APBENR2 |= RCC_APBENR2_SPI1EN;
    (void)RCC->APBENR2;

    /*
     * Keep SPI disabled during configuration.
     */
    SPI1->CR1 &= ~SPI_CR1_SPE;

    /*
     * Master/controller mode.
     *
     * SSM = software slave management.
     * SSI = internal NSS held high.
     *
     * BR = 010:
     * 48 MHz / 8 = 6 MHz SPI clock.
     *
     * CPOL = 0
     * CPHA = 0
     *
     * SPI mode 0.
     */
    SPI1->CR1 =
          SPI_CR1_MSTR
        | SPI_CR1_SSM
        | SPI_CR1_SSI
        | SPI_CR1_BR_1;

    /*
     * 8-bit data frame.
     *
     * DS = 7 means 8-bit frame.
     * FRXTH allows RXNE after 8-bit reception.
     */
    SPI1->CR2 =
          (7U << SPI_CR2_DS_Pos)
        | SPI_CR2_FRXTH;

    /*
     * Enable SPI1.
     */
    SPI1->CR1 |= SPI_CR1_SPE;
}

spi_status_t spi1_transfer(
    uint8_t tx_data,
    uint8_t *rx_data
)
{
    uint32_t timeout;

    /*
     * Validate receive pointer.
     */
    if (rx_data == 0)
    {
        return SPI_STATUS_INVALID_PARAM;
    }

    /*
     * Wait until transmit register is empty.
     */
    timeout = SPI_TIMEOUT;

    while ((SPI1->SR & SPI_SR_TXE) == 0U)
    {
        if (--timeout == 0U)
        {
            return SPI_STATUS_TX_TIMEOUT;
        }
    }

    /*
     * Transmit one byte.
     */
    *((volatile uint8_t *)&SPI1->DR) = tx_data;

    /*
     * Wait until one byte has been received.
     */
    timeout = SPI_TIMEOUT;

    while ((SPI1->SR & SPI_SR_RXNE) == 0U)
    {
        if (--timeout == 0U)
        {
            return SPI_STATUS_RX_TIMEOUT;
        }
    }

    /*
     * Read received byte.
     */
    *rx_data = *((volatile uint8_t *)&SPI1->DR);

    return SPI_STATUS_OK;
}

spi_status_t spi1_loopback_test(uint8_t data)
{
    uint8_t received_data = 0U;
    uint32_t timeout;
    spi_status_t status;

    /*
     * CS LOW:
     * begin transaction.
     */
    GPIOB->BSRR = (1U << 16);

    /*
     * Send byte and receive looped-back byte.
     */
    status = spi1_transfer(
        data,
        &received_data
    );

    if (status != SPI_STATUS_OK)
    {
        /*
         * Always restore CS before returning.
         */
        GPIOB->BSRR = (1U << 0);

        return status;
    }

    /*
     * Wait until SPI hardware finishes the final clock.
     */
    timeout = SPI_TIMEOUT;

    while ((SPI1->SR & SPI_SR_BSY) != 0U)
    {
        if (--timeout == 0U)
        {
            GPIOB->BSRR = (1U << 0);

            return SPI_STATUS_BUSY_TIMEOUT;
        }
    }

    /*
     * CS HIGH:
     * end transaction.
     */
    GPIOB->BSRR = (1U << 0);

    /*
     * Verify loopback result.
     */
    if (received_data != data)
    {
        return SPI_STATUS_LOOPBACK_MISMATCH;
    }

    return SPI_STATUS_OK;
}