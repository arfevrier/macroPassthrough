idf_component_register(SRCS "main.c" "macpass_usb.c" "macpass_spi.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_driver_gpio esp_driver_spi esp_timer
                       REQUIRES boards tinyusb_src
                       )
