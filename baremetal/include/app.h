#ifndef APP_H
#define APP_H

typedef enum
{
    APP_STATE_INIT = 0,
    APP_STATE_READY,
    APP_STATE_RUNNING,
    APP_STATE_WARNING,
    APP_STATE_ERROR
} app_state_t;

typedef enum
{
    APP_ERROR_NONE = 0,
    APP_ERROR_SPI,
    APP_ERROR_I2C_DRIVER,
    APP_ERROR_MPU6050_ID,
    APP_ERROR_ADC,
    APP_ERROR_PWM
} app_error_t;

void app_init(void);
void app_update(void);

#endif