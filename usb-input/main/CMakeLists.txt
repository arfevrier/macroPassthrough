idf_component_register(SRCS "macpass_spi.c" "macpass_usb.c" "main.c"
                       PRIV_REQUIRES usb esp_driver_gpio esp_driver_spi esp_timer
                       INCLUDE_DIRS "")
