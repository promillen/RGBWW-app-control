#ifndef PWM_CONTROL_H
#define PWM_CONTROL_H

#include "driver/ledc.h"

// Board configuration selection
// Define one of these in your build configuration or uncomment the desired one:
// #define ESP32_C3_OLED
#define ESP32_C3_NO_OLED

// PWM Channels
typedef enum {
    PWM_CHANNEL_RED = 0,
    PWM_CHANNEL_GREEN,
    PWM_CHANNEL_BLUE,
    PWM_CHANNEL_WARM_WHITE,
    PWM_CHANNEL_MAX
} pwm_channel_t;

// Board configuration selection
#ifdef ESP32_C3_OLED
    // ESP32-C3 with OLED display - optimized for AL8860
    #define RED_GPIO         GPIO_NUM_10
    #define GREEN_GPIO       GPIO_NUM_9
    #define BLUE_GPIO        GPIO_NUM_8
    #define WARM_WHITE_GPIO  GPIO_NUM_7
    #define BOARD_TYPE       "ESP32-C3 with OLED (AL8860 optimized)"
    
    // AL8860 optimized PWM settings
    #define PWM_FREQUENCY    1000       // Lower frequency for AL8860
    #define PWM_RESOLUTION   LEDC_TIMER_8_BIT  // 8-bit for AL8860
    #define PWM_SPEED_MODE   LEDC_LOW_SPEED_MODE
    
#elif defined(ESP32_C3_NO_OLED)
    // ESP32-C3 without OLED display - optimized for LM3414
    #define RED_GPIO         GPIO_NUM_5
    #define GREEN_GPIO       GPIO_NUM_6
    #define BLUE_GPIO        GPIO_NUM_7
    #define WARM_WHITE_GPIO  GPIO_NUM_8
    #define BOARD_TYPE       "ESP32-C3 without OLED (LM3414 optimized)"
    
    // LM3414 optimized PWM settings (ESP32-C3 only has LOW_SPEED_MODE)
    #define PWM_FREQUENCY    5000       // Higher frequency for LM3414
    #define PWM_RESOLUTION   LEDC_TIMER_12_BIT // 12-bit for LM3414
    #define PWM_SPEED_MODE   LEDC_LOW_SPEED_MODE
    
#else
    // Default to LM3414 configuration
    #define RED_GPIO         GPIO_NUM_5
    #define GREEN_GPIO       GPIO_NUM_6
    #define BLUE_GPIO        GPIO_NUM_7
    #define WARM_WHITE_GPIO  GPIO_NUM_8
    #define BOARD_TYPE       "ESP32-C3 without OLED (LM3414 default)"
    #define PWM_FREQUENCY    5000
    #define PWM_RESOLUTION   LEDC_TIMER_12_BIT
    #define PWM_SPEED_MODE   LEDC_LOW_SPEED_MODE
    #warning "No board type defined, defaulting to LM3414 configuration"
#endif

void pwm_init(void);
void pwm_set_duty(pwm_channel_t channel, uint32_t duty);
void pwm_set_rgbw(uint32_t r, uint32_t g, uint32_t b, uint32_t w);
uint32_t pwm_get_max_duty(void);

#endif