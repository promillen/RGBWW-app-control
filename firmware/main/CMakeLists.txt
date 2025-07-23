idf_component_register(
    SRCS "main.c" "ble_server.c" "pwm_control.c" "light_effects.c" "esp_now_sync.c"
    INCLUDE_DIRS "."
    REQUIRES 
        nvs_flash 
        bt 
        driver 
        log 
        freertos 
        esp_common
        esp_wifi
        esp_timer
    PRIV_REQUIRES
)