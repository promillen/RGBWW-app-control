#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include "esp_gatts_api.h"

// Service and Characteristic UUIDs
#define GATTS_SERVICE_UUID_RGBW         0x00FF
#define GATTS_CHAR_UUID_RED            0xFF01
#define GATTS_CHAR_UUID_GREEN          0xFF02
#define GATTS_CHAR_UUID_BLUE           0xFF03
#define GATTS_CHAR_UUID_WARM_WHITE     0xFF04

// BLE Server functions
void ble_server_init(void);
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#endif