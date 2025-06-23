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

static const char *TAG = "RGBW_MAIN";

#ifdef ESP32_C3_OLED
    #define LED_DRIVER_TYPE "AL8860"
    #define MAX_CURRENT_MA 1500
    #define SWITCHING_FREQ_DEFAULT 250  // AL8860 operates at lower freq (hysteretic)
    #define CURRENT_ACCURACY_PERCENT 5  // AL8860 typical accuracy
    #define DIMMING_RESOLUTION 256      // 8-bit resolution for AL8860
    #define SOFT_START_TIME_MS 1        // AL8860 has adjustable soft-start
#else
    #define LED_DRIVER_TYPE "LM3414"
    #define MAX_CURRENT_MA 1000
    #define SWITCHING_FREQ_DEFAULT 500  // LM3414 can go up to 1MHz
    #define CURRENT_ACCURACY_PERCENT 3  // LM3414 higher accuracy
    #define DIMMING_RESOLUTION 4096     // 12-bit resolution for LM3414
    #define SOFT_START_TIME_MS 0.1      // LM3414 has fast built-in soft-start
#endif

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
    ESP_LOGI(TAG, "BLE advertising as: %s", DEVICE_NAME);
    ESP_LOGI(TAG, "Light effects running - will switch to smooth fade when no device connected");
    ESP_LOGI(TAG, "Ready for connections!");
}