#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

// Service and Characteristic UUIDs
#define RGBW_SERVICE_UUID           0x00FF
#define RGBW_CHAR_UUID_RED          0xFF01
#define RGBW_CHAR_UUID_GREEN        0xFF02
#define RGBW_CHAR_UUID_BLUE         0xFF03
#define RGBW_CHAR_UUID_WARM_WHITE   0xFF04

// Device name - Change this for each device!
// Should match the QR code: RGBW_LED_001, RGBW_LED_002, etc.
#ifndef DEVICE_NAME
#define DEVICE_NAME "RGBW_LED_001"
#endif

// BLE Server functions
void ble_server_init(void);
int ble_gap_event(struct ble_gap_event *event, void *arg);
void ble_advertise(void);
void ble_on_reset(int reason);
void ble_on_sync(void);

#endif