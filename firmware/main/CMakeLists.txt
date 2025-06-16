idf_component_register(
    SRCS "main.c" "ble_server.c" "pwm_control.c"
    INCLUDE_DIRS "."
    REQUIRES 
        nvs_flash 
        bt 
        driver 
        log 
        freertos 
        esp_common
    PRIV_REQUIRES
)