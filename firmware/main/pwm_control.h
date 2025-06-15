#ifndef PWM_CONTROL_H
#define PWM_CONTROL_H

#include "driver/ledc.h"

// PWM Channels
typedef enum {
    PWM_CHANNEL_RED = 0,
    PWM_CHANNEL_GREEN,
    PWM_CHANNEL_BLUE,
    PWM_CHANNEL_WARM_WHITE,
    PWM_CHANNEL_MAX
} pwm_channel_t;

// GPIO assignments
#define RED_GPIO         GPIO_NUM_2
#define GREEN_GPIO       GPIO_NUM_3
#define BLUE_GPIO        GPIO_NUM_4
#define WARM_WHITE_GPIO  GPIO_NUM_5

// PWM Configuration
#define PWM_FREQUENCY    5000
#define PWM_RESOLUTION   LEDC_TIMER_8_BIT
#define PWM_SPEED_MODE   LEDC_LOW_SPEED_MODE

void pwm_init(void);
void pwm_set_duty(pwm_channel_t channel, uint8_t duty);
void pwm_set_rgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w);

#endif