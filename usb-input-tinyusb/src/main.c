// Import global project config
#include "config.h"

void app_main(void)
{
    ESP_LOGI(LOG_TITLE, "Starting -MacroPassthrough- application");

    // Initialize TinyUSB
    usb_initialization();

    // Initialize SPI
    spi_init_master_hid_sender();
    spi_init_slave_pc_receiver();    
    
    // Leave main() in background
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}
