idf_component_register(
    SRCS
        "src/display_hal.cpp"
        "src/circular_clock.cpp"
        "src/dial_renderer.cpp"
        "src/center_renderer.cpp"
        "src/radio_animator.cpp"
        "src/theme.cpp"
        "src/moon_phase.cpp"
        "src/font_instances.cpp"
        "src/locales/locale_data.c"
        "src/clock_settings.c"
    INCLUDE_DIRS
        "include"
    PRIV_INCLUDE_DIRS
        "src"
    REQUIRES
        esp_lcd
        esp_timer
        driver
        freertos
        nvs_flash
)

# C++17 support for C++ files only
target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>)
