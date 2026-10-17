idf_component_register(
    SRCS
        "main.cpp"
        "wifi_sntp_manager.cpp"
        "locale_demo.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_netif
        esp_wifi
        esp_event
        lwip
)
