#ifndef ESP_NOW_SYNC_H
#define ESP_NOW_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_now.h"
#include "light_effects.h"

// Maximum number of peer devices to sync with
#define ESP_NOW_MAX_PEERS 10

// ESP-NOW message types
typedef enum {
    ESP_NOW_MSG_EFFECT_SYNC = 1,
    ESP_NOW_MSG_HEARTBEAT = 2
} esp_now_msg_type_t;

// ESP-NOW message structure for effect synchronization
typedef struct {
    esp_now_msg_type_t msg_type;
    light_effect_t effect;
    uint8_t brightness;
    uint8_t speed;
    uint32_t timestamp;
    uint8_t sender_mac[6];
    uint8_t variance_seed;  // For time variance in effects
} esp_now_effect_msg_t;

// Peer device info
typedef struct {
    uint8_t mac_addr[6];
    bool is_active;
    uint32_t last_seen;
} esp_now_peer_device_t;

// ESP-NOW synchronization functions
esp_err_t esp_now_sync_init(void);
esp_err_t esp_now_sync_deinit(void);
esp_err_t esp_now_add_peer_device(const uint8_t mac_addr[6]);
esp_err_t esp_now_remove_peer_device(const uint8_t mac_addr[6]);
esp_err_t esp_now_broadcast_effect(light_effect_t effect, uint8_t brightness, uint8_t speed);
void esp_now_set_ble_connected(bool connected);
bool esp_now_is_ble_connected(void);
void esp_now_get_mac_address(uint8_t mac[6]);
void esp_now_print_mac_address(void);

// Peer management
esp_err_t esp_now_load_peer_list(void);
esp_err_t esp_now_save_peer_list(void);
int esp_now_get_peer_count(void);
esp_now_peer_device_t* esp_now_get_peer_list(void);

// Time variance for synchronized effects
uint32_t esp_now_get_effect_variance_ms(uint8_t variance_seed);

#endif // ESP_NOW_SYNC_H