#include "stm32c0xx.h"
#include "spi.h"

static void spi1_gpio_init(void)
{
    /* Enable GPIOB clock */
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;

    /*
     * PB3 = SCK
     * PB4 = MISO
     * PB5 = MOSI
     * Configure all three as alternate-function pins.
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

    /* PB0 = normal GPIO output for chip select */
    GPIOB->MODER &= ~(3U << 0);
    GPIOB->MODER |=  (1U << 0);

    /* CS starts HIGH: device inactive */
    GPIOB->BSRR = (1U << 0);
}

void spi1_init(void)
{
    spi1_gpio_init();

    /* Enable SPI1 peripheral clock */
    RCC->APBENR2 |= RCC_APBENR2_SPI1EN;

    /* Keep SPI disabled during configuration */
    SPI1->CR1 &= ~SPI_CR1_SPE;

    /*
     * Controller/master mode
     * Software-controlled CS
     * Internal NSS kept HIGH
     * SPI clock = 48 MHz / 8 = 6 MHz
     * CPOL = 0 and CPHA = 0: SPI mode 0
     */
    SPI1->CR1 = SPI_CR1_MSTR |
                SPI_CR1_SSM  |
                SPI_CR1_SSI  |
                SPI_CR1_BR_1;

    /* 8-bit data frame */
    SPI1->CR2 = (7U << SPI_CR2_DS_Pos) |
                SPI_CR2_FRXTH;

    /* Enable SPI1 */
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t spi1_transfer(uint8_t data)
{
    /* Wait until the transmit register is empty */
    while ((SPI1->SR & SPI_SR_TXE) == 0U)
    {
    }

    /* Transmit one byte through MOSI */
    *((volatile uint8_t *)&SPI1->DR) = data;

    /* Wait until one byte arrives through MISO */
    while ((SPI1->SR & SPI_SR_RXNE) == 0U)
    {
    }

    /* Return the received byte */
    return *((volatile uint8_t *)&SPI1->DR);
}

uint8_t spi1_loopback_test(uint8_t data)
{
    uint8_t received_data;

    /* CS LOW: start transaction */
    GPIOB->BSRR = (1U << 16);

    received_data = spi1_transfer(data);

    /* Wait until the final clock pulse is complete */
    while (SPI1->SR & SPI_SR_BSY)
    {
    }

    /* CS HIGH: end transaction */
    GPIOB->BSRR = (1U << 0);

    return received_data;
}