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

// Board and driver configuration - OVERRIDE Kconfig if needed
#ifdef CONFIG_BOARD_ESP32C3_OLED
    #define BOARD_TYPE       "ESP32-C3 with OLED (AL8860 optimized)"
    #define LED_DRIVER_TYPE  "AL8860"
    
    // Force correct AL8860 settings regardless of Kconfig
    #undef CONFIG_PWM_FREQUENCY_HZ
    #define CONFIG_PWM_FREQUENCY_HZ 1000
    
    #define PWM_FREQUENCY    1000  // AL8860 optimized frequency
    #define PWM_RESOLUTION   LEDC_TIMER_8_BIT
    #define PWM_SPEED_MODE   LEDC_LOW_SPEED_MODE
    
    // Force correct GPIO mapping for AL8860 board
    #undef CONFIG_GPIO_RED
    #undef CONFIG_GPIO_GREEN
    #undef CONFIG_GPIO_BLUE
    #undef CONFIG_GPIO_WARM_WHITE
    #define CONFIG_GPIO_RED 10
    #define CONFIG_GPIO_GREEN 9
    #define CONFIG_GPIO_BLUE 8
    #define CONFIG_GPIO_WARM_WHITE 7
    
    #undef CONFIG_LED_MAX_CURRENT_MA
    #define CONFIG_LED_MAX_CURRENT_MA 1500
    
#elif defined(CONFIG_BOARD_ESP32C3_NO_OLED)
    #define BOARD_TYPE       "ESP32-C3 without OLED (LM3414 optimized)"
    #define LED_DRIVER_TYPE  "LM3414"
    
    // Force correct LM3414 settings regardless of Kconfig
    #undef CONFIG_PWM_FREQUENCY_HZ
    #define CONFIG_PWM_FREQUENCY_HZ 5000
    
    #define PWM_FREQUENCY    5000  // LM3414 optimized frequency
    #define PWM_RESOLUTION   LEDC_TIMER_12_BIT
    #define PWM_SPEED_MODE   LEDC_LOW_SPEED_MODE
    
    // Force correct GPIO mapping for LM3414 board
    #undef CONFIG_GPIO_RED
    #undef CONFIG_GPIO_GREEN
    #undef CONFIG_GPIO_BLUE
    #undef CONFIG_GPIO_WARM_WHITE
    #define CONFIG_GPIO_RED 5
    #define CONFIG_GPIO_GREEN 6
    #define CONFIG_GPIO_BLUE 7
    #define CONFIG_GPIO_WARM_WHITE 8
    
    #undef CONFIG_LED_MAX_CURRENT_MA
    #define CONFIG_LED_MAX_CURRENT_MA 1000
    
#else
    #error "No board type configured. Please run 'idf.py menuconfig' and select a board type."
#endif

// GPIO pin configuration (now forced to correct values)
#define RED_GPIO         CONFIG_GPIO_RED
#define GREEN_GPIO       CONFIG_GPIO_GREEN
#define BLUE_GPIO        CONFIG_GPIO_BLUE
#define WARM_WHITE_GPIO  CONFIG_GPIO_WARM_WHITE
#define MAX_CURRENT_MA   CONFIG_LED_MAX_CURRENT_MA

// Function declarations
void pwm_init(void);
void pwm_set_duty(pwm_channel_t channel, uint32_t duty);
void pwm_set_rgbw(uint32_t r, uint32_t g, uint32_t b, uint32_t w);
uint32_t pwm_get_max_duty(void);

#endif