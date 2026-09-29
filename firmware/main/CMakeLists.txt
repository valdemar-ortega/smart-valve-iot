idf_component_register(
    SRCS
        "app_main.c"
        "wifi_sta.c"
        "pressure_sensor.c"
        "pressure_math.c"
        "mqtt_link.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        esp_wifi
        esp_netif
        esp_event
        esp_adc
        esp_timer
        nvs_flash
        mqtt
        mbedtls
)
