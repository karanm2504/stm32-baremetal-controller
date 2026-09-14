# STM32C031 GPIO Register Notes

## Target

- Board: ST NUCLEO-C031C6
- MCU: STM32C031C6
- Simulation: Wokwi
- User LED: LD4
- LED GPIO: PA5

## GPIO Initialization

Before GPIOA can be used, its peripheral clock must be enabled:

```c
RCC->IOPENR |= (1U << 0);
