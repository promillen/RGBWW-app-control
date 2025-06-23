#include "pwm_control.h"
#include "esp_log.h"

static const char *TAG = "PWM_CONTROL";

void pwm_init(void)
{
    ESP_LOGI(TAG, "Initializing PWM for: %s", BOARD_TYPE);
#ifdef ESP32_C3_OLED
    ESP_LOGI(TAG, "LED Driver: AL8860, Max Current: 1500mA");
#else
    ESP_LOGI(TAG, "LED Driver: LM3414, Max Current: 1000mA");
#endif
    ESP_LOGI(TAG, "GPIO mapping - R:%d, G:%d, B:%d, WW:%d", 
             RED_GPIO, GREEN_GPIO, BLUE_GPIO, WARM_WHITE_GPIO);

    const int gpio_pins[PWM_CHANNEL_MAX] = {
        RED_GPIO,
        GREEN_GPIO,
        BLUE_GPIO,
        WARM_WHITE_GPIO
    };

    // Configure timer with driver-specific settings
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQUENCY,
        .speed_mode = PWM_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configure channels
    for (int i = 0; i < PWM_CHANNEL_MAX; i++) {
        ledc_channel_config_t ledc_channel = {
            .channel = i,
            .duty = 0,
            .gpio_num = gpio_pins[i],
            .speed_mode = PWM_SPEED_MODE,
            .hpoint = 0,
            .timer_sel = LEDC_TIMER_0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }

    ESP_LOGI(TAG, "PWM initialized - Freq: %dHz, Resolution: %d-bit, Max duty: %lu", 
             PWM_FREQUENCY, (PWM_RESOLUTION == LEDC_TIMER_8_BIT) ? 8 : 12, (unsigned long)pwm_get_max_duty());
}

uint32_t pwm_get_max_duty(void)
{
    return (PWM_RESOLUTION == LEDC_TIMER_8_BIT) ? 255 : 4095;
}


void pwm_set_duty(pwm_channel_t channel, uint32_t duty)
{
    if (channel >= PWM_CHANNEL_MAX) {
        ESP_LOGE(TAG, "Invalid PWM channel: %d", channel);
        return;
    }

    uint32_t max_duty = pwm_get_max_duty();
    if (duty > max_duty) {
        ESP_LOGW(TAG, "Duty cycle %lu exceeds maximum %lu, clamping", (unsigned long)duty, (unsigned long)max_duty);
        duty = max_duty;
    }

    ESP_ERROR_CHECK(ledc_set_duty(PWM_SPEED_MODE, channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(PWM_SPEED_MODE, channel));
    
    ESP_LOGD(TAG, "Channel %d set to duty: %lu/%lu (%.1f%%)", 
             channel, (unsigned long)duty, (unsigned long)max_duty, (float)duty * 100.0f / max_duty);
}

void pwm_set_rgbw(uint32_t r, uint32_t g, uint32_t b, uint32_t w)
{
    pwm_set_duty(PWM_CHANNEL_RED, r);
    pwm_set_duty(PWM_CHANNEL_GREEN, g);
    pwm_set_duty(PWM_CHANNEL_BLUE, b);
    pwm_set_duty(PWM_CHANNEL_WARM_WHITE, w);
    
    uint32_t max_duty = pwm_get_max_duty();
    ESP_LOGI(TAG, "RGBW set to: R=%lu, G=%lu, B=%lu, W=%lu (max=%lu)", 
             (unsigned long)r, (unsigned long)g, (unsigned long)b, (unsigned long)w, (unsigned long)max_duty);
}