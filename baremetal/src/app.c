#define MPU6050_ADDR       0x68U
#define MPU6050_WHO_AM_I   0x75U

#include "app.h"
#include "uart.h"
#include "gpio.h"
#include "timebase.h"
#include "adc.h"
#include "pwm.h"
#include "i2c.h"
#include "spi.h"

#include <stdint.h>
#include <string.h>


/* --------------------------------------------------------------------------
 * Application state
 * -------------------------------------------------------------------------- */

static app_state_t app_state = APP_STATE_INIT;
static app_error_t app_error = APP_ERROR_NONE;


/* --------------------------------------------------------------------------
 * UART command handling
 * -------------------------------------------------------------------------- */

static char uart_command[32];
static uint8_t uart_command_index = 0U;


/* --------------------------------------------------------------------------
 * Button debounce
 * -------------------------------------------------------------------------- */

static uint8_t debounce_active = 0U;
static uint32_t debounce_start = 0U;


/* --------------------------------------------------------------------------
 * Runtime values
 * -------------------------------------------------------------------------- */

static uint16_t sensor_value = 0U;
static uint16_t current_pwm_value = 0U;


/* --------------------------------------------------------------------------
 * WARNING-state LED timing
 * -------------------------------------------------------------------------- */

static uint32_t warning_blink_time = 0U;
static uint8_t warning_led_state = 0U;


/* --------------------------------------------------------------------------
 * Internal function prototypes
 * -------------------------------------------------------------------------- */

static void app_raise_error(app_error_t error);
static void app_set_state(app_state_t new_state);
static uint8_t app_button_pressed(void);

static void app_uart_update(void);
static void app_print_status(void);


/* --------------------------------------------------------------------------
 * Application initialization
 * -------------------------------------------------------------------------- */

void app_init(void)
{
    spi_status_t spi_status;
    uint8_t who_am_i = 0U;
    i2c_status_t i2c_status;

    app_state = APP_STATE_INIT;
    app_error = APP_ERROR_NONE;

    sensor_value = 0U;
    current_pwm_value = 0U;

    uart_command_index = 0U;

    uart_init();
    gpio_init();
    timebase_init();
    adc_init();
    pwm_init();
    i2c_init();
    spi1_init();

    uart_write_string("STATE: INIT\r\n");


    /*
     * SPI startup diagnostic.
     *
     * A failed SPI loopback is considered a fatal error.
     */

    spi_status = spi1_loopback_test(0xA5U);

    uart_write_string("SPI status: ");

    switch (spi_status)
    {
        case SPI_STATUS_OK:
            uart_write_string("OK\r\n");
            break;

        case SPI_STATUS_TX_TIMEOUT:
            uart_write_string("TX TIMEOUT\r\n");
            break;

        case SPI_STATUS_RX_TIMEOUT:
            uart_write_string("RX TIMEOUT\r\n");
            break;

        case SPI_STATUS_BUSY_TIMEOUT:
            uart_write_string("BUSY TIMEOUT\r\n");
            break;

        case SPI_STATUS_LOOPBACK_MISMATCH:
            uart_write_string("LOOPBACK MISMATCH\r\n");
            break;

        case SPI_STATUS_INVALID_PARAM:
            uart_write_string("INVALID PARAM\r\n");
            break;

        default:
            uart_write_string("UNKNOWN\r\n");
            break;
    }

    if (spi_status != SPI_STATUS_OK)
    {
        app_raise_error(APP_ERROR_SPI);
        return;
    }


    /*
     * MPU6050 startup diagnostic.
     */

    i2c_status = i2c_read_register(
        MPU6050_ADDR,
        MPU6050_WHO_AM_I,
        &who_am_i
    );


    /*
     * Print detailed I2C driver result.
     */

    uart_write_string("I2C status: ");

    switch (i2c_status)
    {
        case I2C_STATUS_OK:
            uart_write_string("OK\r\n");
            break;

        case I2C_STATUS_BUSY_TIMEOUT:
            uart_write_string("BUSY TIMEOUT\r\n");
            break;

        case I2C_STATUS_TX_TIMEOUT:
            uart_write_string("TX TIMEOUT\r\n");
            break;

        case I2C_STATUS_RX_TIMEOUT:
            uart_write_string("RX TIMEOUT\r\n");
            break;

        case I2C_STATUS_NACK:
            uart_write_string("NACK\r\n");
            break;

        case I2C_STATUS_BUS_ERROR:
            uart_write_string("BUS ERROR\r\n");
            break;

        case I2C_STATUS_ARBITRATION_LOST:
            uart_write_string("ARBITRATION LOST\r\n");
            break;

        case I2C_STATUS_INVALID_PARAM:
            uart_write_string("INVALID PARAM\r\n");
            break;

        default:
            uart_write_string("UNKNOWN\r\n");
            break;
    }


    /*
     * Print MPU6050 identity value.
     */

    uart_write_string("MPU6050 WHO_AM_I: ");
    uart_write_uint16((uint16_t)who_am_i);
    uart_write_string("\r\n");


    /*
     * Transport-level I2C problem:
     * recoverable warning.
     */

    if (i2c_status != I2C_STATUS_OK)
    {
        app_error = APP_ERROR_I2C_DRIVER;
        app_set_state(APP_STATE_WARNING);
        return;
    }


    /*
     * I2C transaction completed successfully,
     * but the returned device identity is incorrect.
     *
     * This is currently observed in Wokwi.
     */

    if (who_am_i != 0x68U)
    {
        app_error = APP_ERROR_MPU6050_ID;
        app_set_state(APP_STATE_WARNING);
        return;
    }
}


/* --------------------------------------------------------------------------
 * Button handling
 * -------------------------------------------------------------------------- */

static uint8_t app_button_pressed(void)
{
    /*
     * EXTI creates the button event.
     *
     * The actual press is verified after a non-blocking
     * 30 ms debounce period.
     */

    if (button_event_take() == 1U)
    {
        if (debounce_active == 0U)
        {
            debounce_active = 1U;
            debounce_start = timebase_millis();
        }
    }

    if (debounce_active == 1U)
    {
        if ((timebase_millis() - debounce_start) >= 30U)
        {
            debounce_active = 0U;

            if (button_is_pressed() == 1U)
            {
                return 1U;
            }
        }
    }

    return 0U;
}


/* --------------------------------------------------------------------------
 * Application state transitions
 * -------------------------------------------------------------------------- */

static void app_set_state(app_state_t new_state)
{
    if (app_state == new_state)
    {
        return;
    }

    app_state = new_state;

    switch (new_state)
    {
        case APP_STATE_INIT:

            uart_write_string("STATE: INIT\r\n");
            break;


        case APP_STATE_READY:

            /*
             * Safe idle state.
             */

            current_pwm_value = 0U;

            pwm_set_duty(0U);
            led_set(0U);

            uart_write_string("STATE: READY\r\n");
            break;


        case APP_STATE_RUNNING:

            /*
             * Normal application operation.
             */

            led_set(1U);

            uart_write_string("STATE: RUNNING\r\n");
            break;


        case APP_STATE_WARNING:

            /*
             * Recoverable fault.
             *
             * Immediately force PWM into its safe state.
             */

            current_pwm_value = 0U;

            pwm_set_duty(0U);

            warning_blink_time = timebase_millis();
            warning_led_state = 0U;

            led_set(0U);

            uart_write_string("STATE: WARNING\r\n");

            if (app_error == APP_ERROR_I2C_DRIVER)
            {
                uart_write_string(
                    "WARNING: I2C driver communication failure\r\n"
                );
            }
            else if (app_error == APP_ERROR_MPU6050_ID)
            {
                uart_write_string(
                    "WARNING: MPU6050 WHO_AM_I mismatch\r\n"
                );
            }

            break;


        case APP_STATE_ERROR:

            /*
             * Fatal fault.
             *
             * Force all controlled outputs into their
             * defined safe state.
             */

            current_pwm_value = 0U;

            pwm_set_duty(0U);
            led_set(1U);

            /*
             * These messages are printed only once,
             * when ERROR is entered.
             */

            uart_write_string("STATE: ERROR\r\n");

            if (app_error == APP_ERROR_SPI)
            {
                uart_write_string(
                    "ERROR: SPI loopback test failed\r\n"
                );
            }
            else if (app_error == APP_ERROR_ADC)
            {
                uart_write_string(
                    "ERROR: ADC value out of range\r\n"
                );
            }
            else if (app_error == APP_ERROR_PWM)
            {
                uart_write_string(
                    "ERROR: PWM duty out of range\r\n"
                );
            }

            break;


        default:
            break;
    }
}


/* --------------------------------------------------------------------------
 * Fatal application error helper
 * -------------------------------------------------------------------------- */

static void app_raise_error(app_error_t error)
{
    app_error = error;
    app_set_state(APP_STATE_ERROR);
}


/* --------------------------------------------------------------------------
 * Print current application status
 * -------------------------------------------------------------------------- */

static void app_print_status(void)
{
    uart_write_string("\r\n");
    uart_write_string("--- SYSTEM STATUS ---\r\n");


    /*
     * Current state.
     */

    uart_write_string("State: ");

    switch (app_state)
    {
        case APP_STATE_INIT:
            uart_write_string("INIT\r\n");
            break;

        case APP_STATE_READY:
            uart_write_string("READY\r\n");
            break;

        case APP_STATE_RUNNING:
            uart_write_string("RUNNING\r\n");
            break;

        case APP_STATE_WARNING:
            uart_write_string("WARNING\r\n");
            break;

        case APP_STATE_ERROR:
            uart_write_string("ERROR\r\n");
            break;

        default:
            uart_write_string("UNKNOWN\r\n");
            break;
    }


    /*
     * ADC value.
     */

    uart_write_string("ADC: ");
    uart_write_uint16(sensor_value);
    uart_write_string(" / 4095\r\n");


    /*
     * PWM compare value.
     */

    uart_write_string("PWM: ");
    uart_write_uint16(current_pwm_value);
    uart_write_string(" / 1000\r\n");


    /*
     * Application error.
     */

    uart_write_string("Error: ");

    switch (app_error)
    {
        case APP_ERROR_NONE:
            uart_write_string("NONE\r\n");
            break;

        case APP_ERROR_SPI:
            uart_write_string("SPI\r\n");
            break;

        case APP_ERROR_I2C_DRIVER:
            uart_write_string("I2C DRIVER\r\n");
            break;

        case APP_ERROR_MPU6050_ID:
            uart_write_string("MPU6050 ID\r\n");
            break;

        case APP_ERROR_ADC:
            uart_write_string("ADC\r\n");
            break;

        case APP_ERROR_PWM:
            uart_write_string("PWM\r\n");
            break;

        default:
            uart_write_string("UNKNOWN\r\n");
            break;
    }

    uart_write_string("---------------------\r\n");
}


/* --------------------------------------------------------------------------
 * UART command processor
 * -------------------------------------------------------------------------- */

static void app_uart_update(void)
{
    char c;

    /*
     * Read all currently available UART characters.
     *
     * uart_read_char_nonblocking() returns immediately,
     * so the state machine continues running even when
     * no UART data is available.
     */

    while (uart_read_char_nonblocking(&c) == 1U)
    {
        /*
         * Enter / newline completes a command.
         */

        if ((c == '\r') || (c == '\n'))
        {
            /*
             * Ignore empty line.
             */

            if (uart_command_index == 0U)
            {
                continue;
            }

            /*
             * Terminate command string.
             */

            uart_command[uart_command_index] = '\0';


            /*
             * STATUS command.
             */

            
            if (strcmp(uart_command, "status") == 0)
{
    app_print_status();
}
else if (strcmp(uart_command, "help") == 0)
{
    uart_write_string("\r\nAvailable commands:\r\n");
    uart_write_string("status  - Show current system status\r\n");
    uart_write_string("help    - Show available commands\r\n");
    uart_write_string("led on  - Turn LED on\r\n");
    uart_write_string("led off - Turn LED off\r\n");
    uart_write_string("start   - Enter RUNNING state\r\n");
    uart_write_string("stop    - Return to READY state\r\n");
}
else if (strcmp(uart_command, "led on") == 0)
{
    led_set(1U);
    uart_write_string("LED: ON\r\n");
}
else if (strcmp(uart_command, "led off") == 0)
{
    led_set(0U);
    uart_write_string("LED: OFF\r\n");
}
else if (strcmp(uart_command, "start") == 0)
{
    if (app_state == APP_STATE_READY)
    {
        app_set_state(APP_STATE_RUNNING);
    }
    else
    {
        uart_write_string("START allowed only from READY\r\n");
    }
}
else if (strcmp(uart_command, "stop") == 0)
{
    if (app_state == APP_STATE_RUNNING)
    {
        app_set_state(APP_STATE_READY);
    }
    else
    {
        uart_write_string("STOP allowed only from RUNNING\r\n");
    }
}
else
{
    uart_write_string("Unknown command: ");
    uart_write_string(uart_command);
    uart_write_string("\r\n");
}


            /*
             * Prepare buffer for next command.
             */

            uart_command_index = 0U;
        }
        else
        {
            /*
             * Add character if space remains.
             */

            if (uart_command_index < (sizeof(uart_command) - 1U))
            {
                uart_command[uart_command_index] = c;
                uart_command_index++;
            }
            else
            {
                /*
                 * Buffer overflow protection.
                 */

                uart_command_index = 0U;

                uart_write_string(
                    "Command too long\r\n"
                );
            }
        }
    }
}


/* --------------------------------------------------------------------------
 * Main application update
 * -------------------------------------------------------------------------- */

void app_update(void)
{
    /*
     * UART command processing is non-blocking.
     */

    app_uart_update();


    /*
     * Application state machine.
     */

    switch (app_state)
    {
        case APP_STATE_INIT:

            /*
             * If startup diagnostics completed without
             * entering WARNING or ERROR, move to READY.
             */

            app_set_state(APP_STATE_READY);
            break;


        case APP_STATE_READY:

            /*
             * Wait for button press before starting.
             */

            if (app_button_pressed() == 1U)
            {
                app_set_state(APP_STATE_RUNNING);
            }

            break;


        case APP_STATE_RUNNING:
        {
            uint32_t pwm_value;


            /*
             * Read potentiometer.
             */

            sensor_value = adc_read();


            /*
             * ADC sanity check.
             */

            if (sensor_value > 4095U)
            {
                app_raise_error(APP_ERROR_ADC);
                break;
            }


            /*
             * Convert 12-bit ADC:
             *
             * 0..4095
             *
             * into PWM compare range:
             *
             * 0..1000
             */

            pwm_value =
                ((uint32_t)sensor_value * 1000U) / 4095U;


            /*
             * PWM sanity check.
             */

            if (pwm_value > 1000U)
            {
                app_raise_error(APP_ERROR_PWM);
                break;
            }


            current_pwm_value = (uint16_t)pwm_value;

            pwm_set_duty(current_pwm_value);


            /*
             * Button stops normal operation
             * and returns to READY.
             */

            if (app_button_pressed() == 1U)
            {
                app_set_state(APP_STATE_READY);
            }

            break;
        }


        case APP_STATE_WARNING:

            /*
             * Non-blocking 500 ms LED blink.
             */

            if ((timebase_millis() - warning_blink_time) >= 500U)
            {
                warning_blink_time = timebase_millis();

                warning_led_state ^= 1U;
                led_set(warning_led_state);
            }


            /*
             * WARNING is recoverable.
             *
             * For the current Wokwi MPU6050 limitation,
             * the button acknowledges the warning and
             * allows operation to continue.
             */

            if (app_button_pressed() == 1U)
            {
                app_error = APP_ERROR_NONE;
                app_set_state(APP_STATE_READY);
            }

            break;


        case APP_STATE_ERROR:

            /*
             * Fatal ERROR remains latched until reset.
             *
             * Do NOT continuously print UART messages here.
             * They were already printed once when entering
             * the ERROR state.
             */

            current_pwm_value = 0U;

            pwm_set_duty(0U);
            led_set(1U);

            break;


        default:

            /*
             * Invalid application state is treated
             * as a fatal application error.
             */

            app_raise_error(APP_ERROR_NONE);
            break;
    }
}