#include "esp_now_sync.h"

#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "light_effects.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "pwm_control.h"
#include "string.h"

static const char *TAG = "ESP_NOW_SYNC";

// ESP-NOW sync state
static bool esp_now_initialized = false;
static bool ble_connected = false;
static esp_now_peer_device_t peer_devices[ESP_NOW_MAX_PEERS];
static int peer_count = 0;
static uint8_t local_mac[6];

// Queue for handling received messages
static QueueHandle_t esp_now_queue = NULL;
#define ESP_NOW_QUEUE_SIZE 10

// Time variance constants (in milliseconds)
#define ESP_NOW_MIN_VARIANCE_MS 0
#define ESP_NOW_MAX_VARIANCE_MS 3000

// NVS keys for peer storage
#define NVS_NAMESPACE "esp_now_sync"
#define NVS_PEER_COUNT_KEY "peer_count"
#define NVS_PEER_PREFIX "peer_"

// Task for processing ESP-NOW messages
static void esp_now_task(void *pvParameter);
static TaskHandle_t esp_now_task_handle = NULL;

// ESP-NOW callback functions
static void esp_now_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "📡 Effect broadcast successful to %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    } else {
        ESP_LOGW(TAG, "📡 Effect broadcast failed to %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    }
}

static void esp_now_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (len != sizeof(esp_now_effect_msg_t)) {
        ESP_LOGW(TAG, "Received invalid message size: %d bytes", len);
        return;
    }

    esp_now_effect_msg_t *msg = (esp_now_effect_msg_t *)data;

    // Log the received message
    ESP_LOGI(TAG, "📨 Received effect sync from %02x:%02x:%02x:%02x:%02x:%02x - Effect: %d, Brightness: %d, Speed: %d",
             recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
             recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5],
             msg->effect, msg->brightness, msg->speed);

    // Only process if we don't have a local BLE connection (local connection has priority)
    if (!ble_connected && esp_now_queue != NULL) {
        if (xQueueSend(esp_now_queue, msg, 0) != pdTRUE) {
            ESP_LOGW(TAG, "ESP-NOW queue full, dropping message");
        }
    } else if (ble_connected) {
        ESP_LOGI(TAG, "🔒 Ignoring remote effect sync - local BLE connection active");
    }
}

static void esp_now_task(void *pvParameter) {
    esp_now_effect_msg_t received_msg;

    while (1) {
        if (xQueueReceive(esp_now_queue, &received_msg, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "🎭 Processing received effect sync: Effect %d", received_msg.effect);

            // Calculate time variance to avoid simultaneous effects
            uint32_t variance_ms = esp_now_get_effect_variance_ms(received_msg.variance_seed);

            if (variance_ms > 0) {
                ESP_LOGI(TAG, "⏱️ Applying %lu ms variance delay for effect synchronization", (unsigned long)variance_ms);
                vTaskDelay(pdMS_TO_TICKS(variance_ms));
            }

            // Apply the received effect
            light_effects_set_effect(received_msg.effect);
            light_effects_set_brightness((uint32_t)received_msg.brightness * pwm_get_max_duty() / 255);
            light_effects_set_speed(received_msg.speed);

            // Disable manual mode for automatic effects
            if (received_msg.effect != EFFECT_STATIC && received_msg.effect != EFFECT_OFF) {
                light_effects_disable_manual_mode();
            }

            ESP_LOGI(TAG, "✨ Applied synchronized effect: %d", received_msg.effect);
        }
    }
}

static esp_err_t esp_now_add_broadcast_peer(void) {
    esp_now_peer_info_t broadcast_peer;

    // Set broadcast MAC address (FF:FF:FF:FF:FF:FF)
    memset(broadcast_peer.peer_addr, 0xFF, 6);
    broadcast_peer.channel = 0;
    broadcast_peer.ifidx = WIFI_IF_STA;
    broadcast_peer.encrypt = false;

    // Add broadcast peer to ESP-NOW
    esp_err_t result = esp_now_add_peer(&broadcast_peer);

    if (result == ESP_OK) {
        ESP_LOGI(TAG, "✅ Broadcast peer (FF:FF:FF:FF:FF:FF) added successfully");
    } else if (result == ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGI(TAG, "✅ Broadcast peer already exists");
        result = ESP_OK;  // Not an error
    } else {
        ESP_LOGE(TAG, "❌ Failed to add broadcast peer: %s", esp_err_to_name(result));
    }

    return result;
}

esp_err_t esp_now_sync_init(void) {
    if (esp_now_initialized) {
        ESP_LOGW(TAG, "ESP-NOW sync already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing ESP-NOW synchronization...");

    // Initialize WiFi in STA mode (required for ESP-NOW)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Get and store local MAC address
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, local_mac));
    ESP_LOGI(TAG, "🏷️ Local MAC address: %02x:%02x:%02x:%02x:%02x:%02x",
             local_mac[0], local_mac[1], local_mac[2], local_mac[3], local_mac[4], local_mac[5]);

    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_cb));

    esp_err_t broadcast_result = esp_now_add_broadcast_peer();
    if (broadcast_result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add broadcast peer, broadcast will not work");
    }

    // Create message processing queue
    esp_now_queue = xQueueCreate(ESP_NOW_QUEUE_SIZE, sizeof(esp_now_effect_msg_t));
    if (esp_now_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create ESP-NOW queue");
        return ESP_ERR_NO_MEM;
    }

    // Create message processing task
    BaseType_t task_created = xTaskCreate(esp_now_task, "esp_now_task", 4096, NULL, 5, &esp_now_task_handle);
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ESP-NOW task");
        vQueueDelete(esp_now_queue);
        return ESP_ERR_NO_MEM;
    }

    // Load peer list from NVS
    esp_now_load_peer_list();

    esp_now_initialized = true;
    ESP_LOGI(TAG, "✅ ESP-NOW synchronization initialized with %d peers", peer_count);

    return ESP_OK;
}

esp_err_t esp_now_sync_deinit(void) {
    if (!esp_now_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing ESP-NOW synchronization...");

    // Stop task and delete queue
    if (esp_now_task_handle != NULL) {
        vTaskDelete(esp_now_task_handle);
        esp_now_task_handle = NULL;
    }

    if (esp_now_queue != NULL) {
        vQueueDelete(esp_now_queue);
        esp_now_queue = NULL;
    }

    // Save peer list
    esp_now_save_peer_list();

    // Deinitialize ESP-NOW
    esp_now_deinit();

    // Stop WiFi
    esp_wifi_stop();
    esp_wifi_deinit();

    esp_now_initialized = false;
    ESP_LOGI(TAG, "ESP-NOW synchronization deinitialized");

    return ESP_OK;
}

esp_err_t esp_now_add_peer_device(const uint8_t mac_addr[6]) {
    if (peer_count >= ESP_NOW_MAX_PEERS) {
        ESP_LOGE(TAG, "Maximum number of peers reached");
        return ESP_ERR_NO_MEM;
    }

    // Check if peer already exists
    for (int i = 0; i < peer_count; i++) {
        if (memcmp(peer_devices[i].mac_addr, mac_addr, 6) == 0) {
            ESP_LOGW(TAG, "Peer already exists: %02x:%02x:%02x:%02x:%02x:%02x",
                     mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
            return ESP_OK;
        }
    }

    // Add new peer to our list
    memcpy(peer_devices[peer_count].mac_addr, mac_addr, 6);
    peer_devices[peer_count].is_active = true;
    peer_devices[peer_count].last_seen = esp_timer_get_time() / 1000;
    peer_count++;

    // Add peer to ESP-NOW
    if (esp_now_initialized) {
        esp_now_peer_info_t peer;
        memcpy(peer.peer_addr, mac_addr, 6);
        peer.channel = 0;
        peer.ifidx = WIFI_IF_STA;
        peer.encrypt = false;

        esp_err_t result = esp_now_add_peer(&peer);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add ESP-NOW peer: %s", esp_err_to_name(result));
            peer_count--;  // Rollback
            return result;
        }
    }

    ESP_LOGI(TAG, "➕ Added peer device: %02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    esp_now_save_peer_list();
    return ESP_OK;
}

esp_err_t esp_now_remove_peer_device(const uint8_t mac_addr[6]) {
    for (int i = 0; i < peer_count; i++) {
        if (memcmp(peer_devices[i].mac_addr, mac_addr, 6) == 0) {
            // Remove from ESP-NOW if initialized
            if (esp_now_initialized) {
                esp_now_del_peer(mac_addr);
            }

            // Shift remaining peers
            for (int j = i; j < peer_count - 1; j++) {
                peer_devices[j] = peer_devices[j + 1];
            }
            peer_count--;

            ESP_LOGI(TAG, "➖ Removed peer device: %02x:%02x:%02x:%02x:%02x:%02x",
                     mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

            esp_now_save_peer_list();
            return ESP_OK;
        }
    }

    ESP_LOGW(TAG, "Peer not found for removal: %02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t esp_now_broadcast_effect(light_effect_t effect, uint8_t brightness, uint8_t speed)
{
    if (!esp_now_initialized || !ble_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "📡 Broadcasting effect %d to ALL devices (brightness: %d, speed: %d)", 
             effect, brightness, speed);

    // Prepare message
    esp_now_effect_msg_t msg;
    msg.msg_type = ESP_NOW_MSG_EFFECT_SYNC;
    msg.effect = effect;
    msg.brightness = brightness;
    msg.speed = speed;
    msg.timestamp = esp_timer_get_time() / 1000;
    memcpy(msg.sender_mac, local_mac, 6);
    msg.variance_seed = esp_random() & 0xFF;

    // Use broadcast MAC address (FF:FF:FF:FF:FF:FF)
    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    esp_err_t result = esp_now_send(broadcast_mac, (uint8_t *)&msg, sizeof(msg));
    
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "📡 Effect broadcast successful to ALL devices");
    } else {
        ESP_LOGW(TAG, "📡 Effect broadcast failed: %s", esp_err_to_name(result));
    }

    return result;
}

void esp_now_set_ble_connected(bool connected) {
    bool prev_connected = ble_connected;
    ble_connected = connected;

    if (connected && !prev_connected) {
        ESP_LOGI(TAG, "🔗 BLE connected - enabling effect broadcasting");
    } else if (!connected && prev_connected) {
        ESP_LOGI(TAG, "🔌 BLE disconnected - disabling effect broadcasting");
    }
}

bool esp_now_is_ble_connected(void) {
    return ble_connected;
}

void esp_now_get_mac_address(uint8_t mac[6]) {
    memcpy(mac, local_mac, 6);
}

void esp_now_print_mac_address(void) {
    ESP_LOGI(TAG, "MAC Address: %02x:%02x:%02x:%02x:%02x:%02x",
             local_mac[0], local_mac[1], local_mac[2], local_mac[3], local_mac[4], local_mac[5]);
}

esp_err_t esp_now_load_peer_list(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No stored peer list found, starting with empty list");
        return ESP_OK;
    }

    // Load peer count
    size_t required_size = sizeof(peer_count);
    err = nvs_get_blob(nvs_handle, NVS_PEER_COUNT_KEY, &peer_count, &required_size);
    if (err != ESP_OK || peer_count > ESP_NOW_MAX_PEERS) {
        peer_count = 0;
        nvs_close(nvs_handle);
        return ESP_OK;
    }

    // Load peer devices
    for (int i = 0; i < peer_count; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%s%d", NVS_PEER_PREFIX, i);

        required_size = sizeof(esp_now_peer_device_t);
        err = nvs_get_blob(nvs_handle, key, &peer_devices[i], &required_size);
        if (err != ESP_OK) {
            peer_count = i;  // Truncate to successfully loaded peers
            break;
        }
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "📂 Loaded %d peers from NVS", peer_count);
    return ESP_OK;
}

esp_err_t esp_now_save_peer_list(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    // Save peer count
    err = nvs_set_blob(nvs_handle, NVS_PEER_COUNT_KEY, &peer_count, sizeof(peer_count));
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    // Save peer devices
    for (int i = 0; i < peer_count; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%s%d", NVS_PEER_PREFIX, i);

        err = nvs_set_blob(nvs_handle, key, &peer_devices[i], sizeof(esp_now_peer_device_t));
        if (err != ESP_OK) {
            nvs_close(nvs_handle);
            return err;
        }
    }

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "💾 Saved %d peers to NVS", peer_count);
    }

    return err;
}

int esp_now_get_peer_count(void) {
    return peer_count;
}

esp_now_peer_device_t *esp_now_get_peer_list(void) {
    return peer_devices;
}

uint32_t esp_now_get_effect_variance_ms(uint8_t variance_seed) {
    // Use variance seed to create deterministic but varied timing
    // This ensures different devices have different delays but same seed gives same delay
    uint32_t max_variance = ESP_NOW_MAX_VARIANCE_MS - ESP_NOW_MIN_VARIANCE_MS;
    uint32_t variance = (variance_seed * max_variance) / 255;
    return ESP_NOW_MIN_VARIANCE_MS + variance;
}

