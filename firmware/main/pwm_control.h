#ifndef PWM_CONTROL_H
#define PWM_CONTROL_H

#include "driver/ledc.h"
#include "sdkconfig.h"

// PWM Channels
typedef enum {
    PWM_CHANNEL_RED = 0,
    PWM_CHANNEL_GREEN,
    PWM_CHANNEL_BLUE,
    PWM_CHANNEL_WARM_WHITE,
    PWM_CHANNEL_MAX
} pwm_channel_t;

// GPIO pin configuration from Kconfig
#define RED_GPIO         CONFIG_GPIO_RED
#define GREEN_GPIO       CONFIG_GPIO_GREEN
#define BLUE_GPIO        CONFIG_GPIO_BLUE
#define WARM_WHITE_GPIO  CONFIG_GPIO_WARM_WHITE

// Board and driver configuration from Kconfig
#ifdef CONFIG_BOARD_ESP32C3_OLED
    #define BOARD_TYPE       "ESP32-C3 with OLED (AL8860 optimized)"
    #define LED_DRIVER_TYPE  "AL8860"
    #define PWM_FREQUENCY    CONFIG_PWM_FREQUENCY_HZ
    #define PWM_RESOLUTION   LEDC_TIMER_8_BIT
    #define PWM_SPEED_MODE   LEDC_LOW_SPEED_MODE
    #define MAX_CURRENT_MA   CONFIG_LED_MAX_CURRENT_MA
    
#elif defined(CONFIG_BOARD_ESP32C3_NO_OLED)
    #define BOARD_TYPE       "ESP32-C3 without OLED (LM3414 optimized)"
    #define LED_DRIVER_TYPE  "LM3414"
    #define PWM_FREQUENCY    CONFIG_PWM_FREQUENCY_HZ
    #define PWM_RESOLUTION   LEDC_TIMER_12_BIT
    #define PWM_SPEED_MODE   LEDC_LOW_SPEED_MODE
    #define MAX_CURRENT_MA   CONFIG_LED_MAX_CURRENT_MA
    
#else
    #error "No board type configured. Please run 'idf.py menuconfig' and select a board type."
#endif

// Function declarations
void pwm_init(void);
void pwm_set_duty(pwm_channel_t channel, uint32_t duty);
void pwm_set_rgbw(uint32_t r, uint32_t g, uint32_t b, uint32_t w);
uint32_t pwm_get_max_duty(void);

#endif