idf_component_register(
    SRCS "main.c" "macpass_hid.c" "macpass_spi.c" "macpass_usb.c" "macpass_macro.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio esp_timer
    )
