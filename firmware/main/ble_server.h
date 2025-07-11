#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sdkconfig.h"

// Service and Characteristic UUIDs
#define RGBW_SERVICE_UUID           0x00FF
#define RGBW_CHAR_UUID_RED          0xFF01
#define RGBW_CHAR_UUID_GREEN        0xFF02
#define RGBW_CHAR_UUID_BLUE         0xFF03
#define RGBW_CHAR_UUID_WARM_WHITE   0xFF04
#define RGBW_CHAR_UUID_EFFECT       0xFF05
#define RGBW_CHAR_UUID_BRIGHTNESS   0xFF06
#define RGBW_CHAR_UUID_SPEED        0xFF07

// Device name from Kconfig
#define DEVICE_NAME CONFIG_DEVICE_NAME

// BLE Server functions
void ble_server_init(void);
int ble_gap_event(struct ble_gap_event *event, void *arg);
void ble_advertise(void);
void ble_on_reset(int reason);
void ble_on_sync(void);

#endif