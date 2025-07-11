#include "ble_server.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "light_effects.h"
#include "nimble/hci_common.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "pwm_control.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BLE_SERVER";

static uint8_t own_addr_type;

// GATT characteristic access functions
static int rgbw_red_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);
static int rgbw_green_access(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg);
static int rgbw_blue_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);
static int rgbw_white_access(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg);
static int rgbw_effect_access(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg);
static int rgbw_brightness_access(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg);
static int rgbw_speed_access(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg);

// GATT service definition
static const struct ble_gatt_svc_def gatt_svc_def[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(RGBW_SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = BLE_UUID16_DECLARE(RGBW_CHAR_UUID_RED),
                .access_cb = rgbw_red_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(RGBW_CHAR_UUID_GREEN),
                .access_cb = rgbw_green_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(RGBW_CHAR_UUID_BLUE),
                .access_cb = rgbw_blue_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(RGBW_CHAR_UUID_WARM_WHITE),
                .access_cb = rgbw_white_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(RGBW_CHAR_UUID_EFFECT),
                .access_cb = rgbw_effect_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(RGBW_CHAR_UUID_BRIGHTNESS),
                .access_cb = rgbw_brightness_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(RGBW_CHAR_UUID_SPEED),
                .access_cb = rgbw_speed_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(RGBW_CHAR_UUID_CHIP_INFO),
                .access_cb = rgbw_chip_info_access,
                .flags = BLE_GATT_CHR_F_READ, // Read-only
            },
            {
                0, /* No more characteristics in this service */
            },
        }
    },
    {
        0, /* No more services */
    },
};

// Current RGBW values (stored as 8-bit values for BLE)
static uint8_t current_rgbw[4] = {0, 0, 0, 0};

// Helper function to convert 8-bit BLE value to driver resolution
static uint32_t convert_to_driver_resolution(uint8_t ble_value) {
    uint32_t max_duty = pwm_get_max_duty();
    return (ble_value * max_duty) / 255;
}

// Helper function to convert driver resolution to 8-bit BLE value
static uint8_t convert_from_driver_resolution(uint32_t driver_value) {
    uint32_t max_duty = pwm_get_max_duty();
    return (driver_value * 255) / max_duty;
}

static int rgbw_red_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            rc = os_mbuf_append(ctxt->om, &current_rgbw[0], sizeof(uint8_t));
            ESP_LOGI(TAG, "Red read: %d", current_rgbw[0]);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            rc = ble_hs_mbuf_to_flat(ctxt->om, &current_rgbw[0], sizeof(uint8_t), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            // Enable manual mode when individual colors are set
            light_effects_enable_manual_mode();
            light_effects_set_effect(EFFECT_STATIC);

            // Convert 8-bit BLE value to driver resolution
            uint32_t driver_value = convert_to_driver_resolution(current_rgbw[0]);
            pwm_set_duty(PWM_CHANNEL_RED, driver_value);
            ESP_LOGI(TAG, "🔴 Red set to: %d (driver: %lu)", current_rgbw[0], (unsigned long)driver_value);
            return 0;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}

static int rgbw_green_access(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            rc = os_mbuf_append(ctxt->om, &current_rgbw[1], sizeof(uint8_t));
            ESP_LOGI(TAG, "Green read: %d", current_rgbw[1]);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            rc = ble_hs_mbuf_to_flat(ctxt->om, &current_rgbw[1], sizeof(uint8_t), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            // Enable manual mode when individual colors are set
            light_effects_enable_manual_mode();
            light_effects_set_effect(EFFECT_STATIC);

            // Convert 8-bit BLE value to driver resolution
            uint32_t driver_value = convert_to_driver_resolution(current_rgbw[1]);
            pwm_set_duty(PWM_CHANNEL_GREEN, driver_value);
            ESP_LOGI(TAG, "🟢 Green set to: %d (driver: %lu)", current_rgbw[1], (unsigned long)driver_value);
            return 0;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}

static int rgbw_blue_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            rc = os_mbuf_append(ctxt->om, &current_rgbw[2], sizeof(uint8_t));
            ESP_LOGI(TAG, "Blue read: %d", current_rgbw[2]);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            rc = ble_hs_mbuf_to_flat(ctxt->om, &current_rgbw[2], sizeof(uint8_t), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            // Enable manual mode when individual colors are set
            light_effects_enable_manual_mode();
            light_effects_set_effect(EFFECT_STATIC);

            // Convert 8-bit BLE value to driver resolution
            uint32_t driver_value = convert_to_driver_resolution(current_rgbw[2]);
            pwm_set_duty(PWM_CHANNEL_BLUE, driver_value);
            ESP_LOGI(TAG, "🔵 Blue set to: %d (driver: %lu)", current_rgbw[2], (unsigned long)driver_value);
            return 0;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}

static int rgbw_white_access(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            rc = os_mbuf_append(ctxt->om, &current_rgbw[3], sizeof(uint8_t));
            ESP_LOGI(TAG, "White read: %d", current_rgbw[3]);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            rc = ble_hs_mbuf_to_flat(ctxt->om, &current_rgbw[3], sizeof(uint8_t), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            // Enable manual mode when individual colors are set
            light_effects_enable_manual_mode();
            light_effects_set_effect(EFFECT_STATIC);

            // Convert 8-bit BLE value to driver resolution
            uint32_t driver_value = convert_to_driver_resolution(current_rgbw[3]);
            pwm_set_duty(PWM_CHANNEL_WARM_WHITE, driver_value);
            ESP_LOGI(TAG, "⚪ Warm White set to: %d (driver: %lu)", current_rgbw[3], (unsigned long)driver_value);
            return 0;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}

static int rgbw_effect_access(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;
    uint8_t effect_value;

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            effect_value = (uint8_t)light_effects_get_current_effect();
            rc = os_mbuf_append(ctxt->om, &effect_value, sizeof(uint8_t));
            ESP_LOGI(TAG, "Effect read: %d", effect_value);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            rc = ble_hs_mbuf_to_flat(ctxt->om, &effect_value, sizeof(uint8_t), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            if (effect_value >= EFFECT_MAX) {
                ESP_LOGW(TAG, "Invalid effect value: %d (max: %d)", effect_value, EFFECT_MAX - 1);
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            // Set the effect
            light_effects_set_effect((light_effect_t)effect_value);

            // If setting a non-static effect, disable manual mode
            if (effect_value != EFFECT_STATIC && effect_value != EFFECT_OFF) {
                light_effects_disable_manual_mode();
            } else {
                light_effects_enable_manual_mode();
            }

            ESP_LOGI(TAG, "✨ Effect set to: %d", effect_value);
            return 0;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}

static int rgbw_brightness_access(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;
    uint8_t brightness_value;

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            // Convert current brightness from driver resolution to 8-bit for BLE
            brightness_value = convert_from_driver_resolution(light_effects_get_config()->brightness);
            rc = os_mbuf_append(ctxt->om, &brightness_value, sizeof(uint8_t));
            ESP_LOGI(TAG, "Brightness read: %d", brightness_value);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            rc = ble_hs_mbuf_to_flat(ctxt->om, &brightness_value, sizeof(uint8_t), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            // Convert 8-bit BLE value to driver resolution
            uint32_t driver_brightness = convert_to_driver_resolution(brightness_value);
            light_effects_set_brightness(driver_brightness);
            ESP_LOGI(TAG, "🔆 Brightness set to: %d (driver: %lu)", brightness_value, (unsigned long)driver_brightness);
            return 0;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}

static int rgbw_speed_access(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;
    uint8_t speed_value;

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            speed_value = light_effects_get_config()->speed;
            rc = os_mbuf_append(ctxt->om, &speed_value, sizeof(uint8_t));
            ESP_LOGI(TAG, "Speed read: %d", speed_value);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            rc = ble_hs_mbuf_to_flat(ctxt->om, &speed_value, sizeof(uint8_t), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            light_effects_set_speed(speed_value);
            ESP_LOGI(TAG, "⚡ Speed set to: %d", speed_value);
            return 0;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}

int ble_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "connection %s; status=%d",
                     event->connect.status == 0 ? "established" : "failed",
                     event->connect.status);
            if (event->connect.status == 0) {
                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                ESP_LOGI(TAG, "🔗 Connected to %02x:%02x:%02x:%02x:%02x:%02x",
                         desc.peer_id_addr.val[0], desc.peer_id_addr.val[1],
                         desc.peer_id_addr.val[2], desc.peer_id_addr.val[3],
                         desc.peer_id_addr.val[4], desc.peer_id_addr.val[5]);

                // Notify effects system about BLE connection
                light_effects_set_ble_connected(true);
            }
            if (event->connect.status != 0) {
                /* Connection failed; resume advertising */
                ble_advertise();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "🔌 disconnect; reason=%d", event->disconnect.reason);

            // Notify effects system about BLE disconnection
            light_effects_set_ble_connected(false);

            /* Connection terminated; resume advertising */
            ble_advertise();
            return 0;

        case BLE_GAP_EVENT_CONN_UPDATE:
            ESP_LOGI(TAG, "connection updated; status=%d",
                     event->conn_update.status);
            rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
            assert(rc == 0);
            ESP_LOGI(TAG, "Updated connection params: interval=%d, latency=%d, timeout=%d",
                     desc.conn_itvl, desc.conn_latency, desc.supervision_timeout);
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "advertise complete; reason=%d",
                     event->adv_complete.reason);
            ble_advertise();
            return 0;

        case BLE_GAP_EVENT_ENC_CHANGE:
            ESP_LOGI(TAG, "encryption change event; status=%d",
                     event->enc_change.status);
            rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
            assert(rc == 0);
            return 0;

        case BLE_GAP_EVENT_NOTIFY_TX:
            ESP_LOGI(TAG,
                     "notify_tx event; conn_handle=%d attr_handle=%d "
                     "status=%d is_indication=%d",
                     event->notify_tx.conn_handle,
                     event->notify_tx.attr_handle,
                     event->notify_tx.status,
                     event->notify_tx.indication);
            return 0;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG,
                     "subscribe event; conn_handle=%d attr_handle=%d "
                     "reason=%d prevn=%d curn=%d previ=%d curi=%d",
                     event->subscribe.conn_handle,
                     event->subscribe.attr_handle,
                     event->subscribe.reason,
                     event->subscribe.prev_notify,
                     event->subscribe.cur_notify,
                     event->subscribe.prev_indicate,
                     event->subscribe.cur_indicate);
            return 0;

        case BLE_GAP_EVENT_MTU:
            ESP_LOGI(TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d",
                     event->mtu.conn_handle,
                     event->mtu.channel_id,
                     event->mtu.value);
            return 0;

        default:
            return 0;
    }
}

void ble_advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;

    /* Configure advertisement parameters */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = BLE_GAP_ADV_FAST_INTERVAL1_MIN;
    adv_params.itvl_max = BLE_GAP_ADV_FAST_INTERVAL1_MAX;

    /* Configure advertisement data */
    memset(&fields, 0, sizeof fields);

    /* Advertise device name */
    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    /* Advertise the service UUID */
    fields.uuids16 = (ble_uuid16_t[]){
        BLE_UUID16_INIT(RGBW_SERVICE_UUID)};
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    /* Set advertisement flags */
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Add manufacturer specific data with chip type information */
    uint8_t mfg_data[3];
    mfg_data[0] = 0xFF;  // Company ID (using 0xFF for custom/test)
    mfg_data[1] = 0xFF;  // Company ID continued
#ifdef CONFIG_BOARD_ESP32C3_OLED
    mfg_data[2] = 0xA8;  // AL8860 identifier
#elif defined(CONFIG_BOARD_ESP32C3_NO_OLED)
    mfg_data[2] = 0x34;  // LM3414 identifier
#else
    mfg_data[2] = 0x00;  // Unknown
#endif

    fields.mfg_data = mfg_data;
    fields.mfg_data_len = sizeof(mfg_data);

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "error setting advertisement data; rc=%d", rc);
        return;
    }

    /* Begin advertising */
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "error enabling advertisement; rc=%d", rc);
        return;
    }

    ESP_LOGI(TAG, "📡 advertising started with chip type: %s",
#ifdef CONFIG_BOARD_ESP32C3_OLED
             "AL8860"
#elif defined(CONFIG_BOARD_ESP32C3_NO_OLED)
             "LM3414"
#else
             "Unknown"
#endif
    );
}

void ble_on_reset(int reason) {
    ESP_LOGE(TAG, "resetting state; reason=%d", reason);
}

void ble_on_sync(void) {
    int rc;

    /* Make sure we have proper identity address set (public preferred) */
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "error determining address type; rc=%d", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

    ESP_LOGI(TAG, "📱 Device Address: %02x:%02x:%02x:%02x:%02x:%02x",
             addr_val[5], addr_val[4], addr_val[3],
             addr_val[2], addr_val[1], addr_val[0]);

    /* Begin advertising */
    ESP_LOGI(TAG, "BLE sync completed");
    ble_advertise();
}

void ble_host_task(void *param) {
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();  // This function will return only when nimble_port_stop() is executed
    nimble_port_freertos_deinit();
}

void ble_server_init(void) {
    int rc;

    /* Initialize the NimBLE host configuration */
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.gatts_register_cb = NULL;
    ble_hs_cfg.store_status_cb = NULL;

    /* Set device name */
    rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    assert(rc == 0);

    /* Initialize GATT services */
    rc = ble_gatts_count_cfg(gatt_svc_def);
    if (rc != 0) {
        ESP_LOGE(TAG, "error counting GATT services; rc=%d", rc);
        return;
    }

    rc = ble_gatts_add_svcs(gatt_svc_def);
    if (rc != 0) {
        ESP_LOGE(TAG, "error adding GATT services; rc=%d", rc);
        return;
    }

    /* Initialize and start the BLE host task */
    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE server initialized");
}

static int rgbw_chip_info_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;
    uint8_t chip_info[16];  // Buffer for chip info string

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
#ifdef CONFIG_BOARD_ESP32C3_OLED
            strcpy((char *)chip_info, "AL8860");
#elif defined(CONFIG_BOARD_ESP32C3_NO_OLED)
            strcpy((char *)chip_info, "LM3414");
#else
            strcpy((char *)chip_info, "UNKNOWN");
#endif
            rc = os_mbuf_append(ctxt->om, chip_info, strlen((char *)chip_info));
            ESP_LOGI(TAG, "Chip info read: %s", chip_info);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            // Read-only characteristic
            return BLE_ATT_ERR_WRITE_NOT_PERMITTED;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}