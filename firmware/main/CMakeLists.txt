idf_component_register(SRCS "main.c" "ble_server.c" "pwm_control.c" "wifi_manager.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_wifi esp_http_server bt)