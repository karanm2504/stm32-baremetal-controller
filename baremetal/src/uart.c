#include "stm32c0xx.h"
#include "uart.h"

void uart_init(void)
{
    /* Enable GPIOA clock */
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    /* PA2 and PA3 = alternate-function mode */
    GPIOA->MODER &= ~((3U << 4) | (3U << 6));
    GPIOA->MODER |=  ((2U << 4) | (2U << 6));

    /* PA2 = USART2_TX AF1, PA3 = USART2_RX AF1 */
    GPIOA->AFR[0] &= ~((0xFU << 8) | (0xFU << 12));
    GPIOA->AFR[0] |=  ((1U << 8) | (1U << 12));

    /* Enable USART2 clock */
    RCC->APBENR1 |= RCC_APBENR1_USART2EN;

    /* Keep USART disabled while configuring */
    USART2->CR1 = 0U;

    /* Configure 115200 baud using the 48 MHz clock */
    USART2->BRR = (SystemCoreClock + 57600U) / 115200U;

    /* Enable transmitter, receiver and USART2 */
    USART2->CR1 = USART_CR1_TE |
                  USART_CR1_RE |
                  USART_CR1_UE;
}

void uart_write_char(char c)
{
    while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0U)
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
    while ((USART2->ISR & USART_ISR_RXNE_RXFNE) == 0U)
    {
    }

    return (char)USART2->RDR;
}

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