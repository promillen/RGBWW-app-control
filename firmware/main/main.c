#include <stdio.h>
#include <string.h>

#include "ble_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs_flash.h"
#include "pwm_control.h"
#include "light_effects.h"
#include "sdkconfig.h"

static const char *TAG = "RGBW_MAIN";

void app_main(void) {
    esp_err_t ret;

    /* Initialize NVS — it is used to store PHY calibration data */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "=== RGBW LED Controller ===");
    ESP_LOGI(TAG, "Board: %s", BOARD_TYPE);
    ESP_LOGI(TAG, "LED Driver: %s", LED_DRIVER_TYPE);
    ESP_LOGI(TAG, "Max Current: %dmA", CONFIG_LED_MAX_CURRENT_MA);
    ESP_LOGI(TAG, "PWM Frequency: %dHz", CONFIG_PWM_FREQUENCY_HZ);
    
    // Log GPIO configuration
    ESP_LOGI(TAG, "GPIO Configuration:");
    ESP_LOGI(TAG, "  Red: GPIO %d", CONFIG_GPIO_RED);
    ESP_LOGI(TAG, "  Green: GPIO %d", CONFIG_GPIO_GREEN);
    ESP_LOGI(TAG, "  Blue: GPIO %d", CONFIG_GPIO_BLUE);
    ESP_LOGI(TAG, "  Warm White: GPIO %d", CONFIG_GPIO_WARM_WHITE);

    /* Initialize PWM for LED control */
    pwm_init();

    /* Initialize light effects system */
    light_effects_init();
    light_effects_start();

    ESP_LOGI(TAG, "Starting NimBLE BLE stack");

    /* Initialize NimBLE */
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble %d ", ret);
        return;
    }

    /* Initialize BLE server */
    ble_server_init();

    ESP_LOGI(TAG, "RGBW LED Controller started");
    ESP_LOGI(TAG, "BLE advertising as: %s", CONFIG_DEVICE_NAME);
    
    // Board-specific startup messages
#ifdef CONFIG_BOARD_ESP32C3_OLED
    ESP_LOGI(TAG, "AL8860 driver optimizations active (8-bit PWM, %dHz)", CONFIG_PWM_FREQUENCY_HZ);
#elif defined(CONFIG_BOARD_ESP32C3_NO_OLED)
    ESP_LOGI(TAG, "LM3414 driver optimizations active (12-bit PWM, %dHz)", CONFIG_PWM_FREQUENCY_HZ);
#endif
    
    ESP_LOGI(TAG, "Light effects running - will switch to smooth fade when no device connected");
    ESP_LOGI(TAG, "Ready for connections!");
}