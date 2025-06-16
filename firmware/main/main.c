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

static const char *TAG = "RGBW_MAIN";

void app_main(void) {
    int rc;

    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize PWM for LED control */
    pwm_init();

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
    ESP_LOGI(TAG, "Ready for connections!");
}