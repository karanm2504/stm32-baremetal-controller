# STM32 Bare-Metal Embedded Controller & Diagnostic Firmware

A register-level embedded firmware project for the **STM32C031C6 ARM Cortex-M0+** microcontroller.

The project demonstrates peripheral configuration, hardware control, communication protocols and diagnostic techniques without using the STM32 HAL.

## Project Status

| Feature | Status |
|---|---|
| GPIO input and output | Completed |
| EXTI button interrupt | Completed |
| SysTick timebase and debounce | Completed |
| UART diagnostics | Completed |
| ADC potentiometer reading | Completed |
| TIM3 PWM generation | Completed |
| SPI1 driver and loopback test | Completed |
| I²C1 register-read driver | Implemented; simulator issue under investigation |
| Application state machine | Next step |
| Final error handling | Planned |

## Implemented Features

### GPIO and Interrupts

- GPIO register configuration
- User-button input
- LED output control
- Rising and falling-edge EXTI interrupts
- SysTick-based button debounce
- Atomic GPIO control using the BSRR register

### UART Diagnostics

- USART2 configured for `115200 8N1`
- String and integer transmission
- Runtime status messages
- Peripheral troubleshooting and stage markers
- Non-blocking receive experiments

### ADC

- 12-bit ADC conversion
- Potentiometer input
- End-of-conversion flag handling
- ADC values transmitted through UART
- ADC value mapped to PWM duty cycle

### Timers and PWM

- TIM3 configured for PWM generation
- 48 MHz system clock
- 1 MHz timer counter
- 500 Hz PWM output
- Adjustable duty cycle through the CCR register

### SPI

- SPI1 controller configuration
- SCK, MOSI, MISO and software-controlled CS
- SPI mode 0
- 8-bit full-duplex transfers
- Loopback transfer testing
- Logic-analyzer verification

### I²C

- I²C1 register-level configuration
- Open-drain SDA and SCL pins
- External 4.7 kΩ pull-up resistors
- MPU6050 device address: `0x68`
- `WHO_AM_I` register address: `0x75`
- ACK/NACK and transaction-stage diagnostics

The I²C driver is retained in the project, but the MPU6050 `WHO_AM_I` test has not been completed successfully in Wokwi. Logic-analyzer captures indicate a simulator-specific transaction issue. Verification on physical hardware remains planned.

## Pin Configuration

| Function | STM32 pin |
|---|---|
| User button | PA0 |
| Potentiometer ADC | PA1 |
| USART2 TX | PA2 |
| USART2 RX | PA3 |
| LD4 LED | PA5 |
| TIM3 PWM output | PA6 |
| SPI1 SCK | PB3 |
| SPI1 MISO | PB4 |
| SPI1 MOSI | PB5 |
| SPI chip select | PB0 |
| I²C1 SCL | PB8 |
| I²C1 SDA | PB9 |

## Project Structure

```text
stm32-baremetal-controller/
└── baremetal/
    ├── include/                Driver header files
    ├── src/                    Application and driver source files
    ├── startup_stm32c0xx.s     Startup code and vector table
    ├── system_stm32c0xx.c      System clock definitions
    ├── flash.ld                Linker script
    └── Makefile                Build configuration
```

## Building the Firmware

### Requirements

- GNU Make
- `arm-none-eabi-gcc`
- WSL or Linux
- Wokwi for simulation

### Build Commands

From the repository root:

```bash
make -C baremetal clean
make -C baremetal
```

Alternatively, from inside the `baremetal` directory:

```bash
make clean
make
```

## Development Environment

- STM32 Nucleo-C031C6
- ARM Cortex-M0+
- C / Embedded C
- CMSIS device headers
- Direct peripheral-register access
- GNU Arm Embedded Toolchain
- Make
- Git and GitHub
- Wokwi simulator
- Logic analyzer
- VS Code with WSL

## Design Goals

- Understand STM32 peripherals at register level
- Avoid high-level HAL abstractions
- Build reusable peripheral drivers
- Apply modular firmware architecture
- Add UART-based diagnostics
- Implement timeouts and safe error handling
- Create a portfolio-ready embedded firmware project

## Next Steps

- Implement the application state machine:
  - `INIT`
  - `READY`
  - `RUNNING`
  - `WARNING`
  - `ERROR`
- Add consistent driver error codes
- Define safe PWM and LED behavior during faults
- Integrate all drivers into the main controller
- Test complete application behaviour
- Verify I²C communication on physical hardware
- Add architecture and state-machine diagrams
- Document final test results

## Current Limitation

The MPU6050 `WHO_AM_I` transaction does not currently return the expected value in the Wokwi simulation. The firmware confirms that `0x75` reaches the I²C driver, but the captured simulated transaction does not transmit the expected register byte.

This limitation is documented instead of hiding the unsuccessful test. The driver will be tested again using physical STM32 hardware.

