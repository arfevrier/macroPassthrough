// Import global project config
#include "config.h"

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000)); // At sleep in case of computer boot
    ESP_LOGI(LOG_TITLE, "Starting -MacroPassthrough- application");

    // Initialize SPI
    spi_init_slave_hid_receiver();
    spi_init_master_pc_sender();

    // Initialize hid multiplexer worker (aggregate keyboard report & macro report)
    hid_init_multiplexer();

    // Initialize macro configuration
    macro_init();

    // Initialize TinyUSB
    tud_user_initialization();

    // Leave main() in background
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}
