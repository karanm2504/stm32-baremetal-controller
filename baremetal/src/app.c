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

static app_state_t app_state = APP_STATE_INIT;
static app_error_t app_error = APP_ERROR_NONE;

static uint8_t debounce_active = 0U;
static uint32_t debounce_start = 0U;

static uint16_t sensor_value = 0U;

static uint32_t warning_blink_time = 0U;
static uint8_t warning_led_state = 0U;

static void app_raise_error(app_error_t error);
static void app_set_state(app_state_t new_state);
static uint8_t app_button_pressed(void);

void app_init(void)
{
    spi_status_t spi_status;
    uint8_t who_am_i = 0U;
    i2c_status_t i2c_status;

    app_state = APP_STATE_INIT;
    app_error = APP_ERROR_NONE;

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
     * A failed SPI loopback is treated as a fatal error.
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
     * I2C transaction completed, but device identity was wrong.
     * This is currently expected in Wokwi.
     */
    if (who_am_i != 0x68U)
    {
        app_error = APP_ERROR_MPU6050_ID;
        app_set_state(APP_STATE_WARNING);
        return;
    }
}

static uint8_t app_button_pressed(void)
{
    /*
     * EXTI interrupt creates a button event.
     * This function performs non-blocking debounce.
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
            led_set(0U);
            pwm_set_duty(0U);

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
             * Recoverable fault:
             * PWM immediately enters safe state.
             */
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
             * Fatal fault:
             * force outputs to safe state.
             */
            pwm_set_duty(0U);
            led_set(1U);

            uart_write_string("STATE: ERROR\r\n");

            if (app_error == APP_ERROR_SPI)
            {
                uart_write_string(
                    "ERROR: SPI loopback test failed\r\n"
                );
            }

            break;

        default:
            break;
    }
}

static void app_raise_error(app_error_t error)
{
    app_error = error;
    app_set_state(APP_STATE_ERROR);
}

void app_update(void)
{
    switch (app_state)
    {
        case APP_STATE_INIT:
            /*
             * If startup tests completed without changing
             * the state to WARNING or ERROR, enter READY.
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
            if (sensor_value > 4095U)
    {
    app_raise_error(APP_ERROR_ADC);
    break;
    }
            /*
             * Convert 12-bit ADC:
             * 0..4095
             *
             * into timer CCR:
             * 0..1000
             */
            pwm_value =
                ((uint32_t)sensor_value * 1000U) / 4095U;

            pwm_set_duty((uint16_t)pwm_value);

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
             * Warning is recoverable.
             *
             * For the current Wokwi I2C limitation,
             * allow the user to acknowledge the warning
             * and continue to READY.
             */
            if (app_button_pressed() == 1U)
            {
                app_error = APP_ERROR_NONE;
                app_set_state(APP_STATE_READY);
            }

            break;

       case APP_STATE_ERROR:
    pwm_set_duty(0U);
    led_set(1U);

    uart_write_string("STATE: ERROR\r\n");

    if (app_error == APP_ERROR_SPI)
    {
        uart_write_string("ERROR: SPI loopback test failed\r\n");
    }
    else if (app_error == APP_ERROR_ADC)
    {
        uart_write_string("ERROR: ADC value out of range\r\n");
    }

    break;

        default:
            /*
             * Invalid application state is treated
             * as a fatal error.
             */
            app_raise_error(APP_ERROR_NONE);
            break;
    }
}