#include "stm32c0xx.h"
#include "timebase.h"

static volatile uint32_t system_ms = 0U;

void SysTick_Handler(void)
{
    system_ms++;
}

void timebase_init(void)
{
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);
}

uint32_t timebase_millis(void)
{
    return system_ms;
}