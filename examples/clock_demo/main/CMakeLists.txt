idf_component_register(
    SRCS
        "main.cpp"
        "wifi_sntp_manager.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_netif
        esp_event
        lwip
)
