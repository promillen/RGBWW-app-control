#include "pwm_control.h"
#include "esp_log.h"

static const char *TAG = "PWM_CONTROL";

static const int gpio_pins[PWM_CHANNEL_MAX] = {
    RED_GPIO,
    GREEN_GPIO,
    BLUE_GPIO,
    WARM_WHITE_GPIO
};

void pwm_init(void)
{
    // Configure timer
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

    ESP_LOGI(TAG, "PWM initialized for RGBW control");
}

void pwm_set_duty(pwm_channel_t channel, uint8_t duty)
{
    if (channel >= PWM_CHANNEL_MAX) {
        ESP_LOGE(TAG, "Invalid PWM channel: %d", channel);
        return;
    }

    ESP_ERROR_CHECK(ledc_set_duty(PWM_SPEED_MODE, channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(PWM_SPEED_MODE, channel));
    
    ESP_LOGD(TAG, "Channel %d set to duty: %d", channel, duty);
}

void pwm_set_rgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    pwm_set_duty(PWM_CHANNEL_RED, r);
    pwm_set_duty(PWM_CHANNEL_GREEN, g);
    pwm_set_duty(PWM_CHANNEL_BLUE, b);
    pwm_set_duty(PWM_CHANNEL_WARM_WHITE, w);
    
    ESP_LOGI(TAG, "RGBW set to: R=%d, G=%d, B=%d, W=%d", r, g, b, w);
}