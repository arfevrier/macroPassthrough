// Import global project config
#include "config.h"

spi_slave_transaction_t spi_transaction_hid_receiver;
spi_hid_transmit_t* spi_hid_buffer;

spi_transaction_t spi_transaction_pc_sender;
spi_pc_transmit_t* spi_pc_buffer;
spi_device_handle_t spi_handle_pc_sender;

// Task to handle SPI data reception from the HID multiplexer
void spi_task_slave_hid_receiver(void *pvParameters){
    while(true){
        // Clear the buffer for new transaction
        memset(spi_hid_buffer, 0, sizeof(spi_hid_transmit_t));
        spi_transaction_hid_receiver.length = sizeof(spi_hid_transmit_t) * 8;
        spi_transaction_hid_receiver.tx_buffer = NULL;
        spi_transaction_hid_receiver.rx_buffer = spi_hid_buffer;

        // Wait for the next SPI transmission
        esp_err_t ret = spi_slave_transmit(SPI_HID_RECEIVER, &spi_transaction_hid_receiver, portMAX_DELAY);

        // Validate the received buffer
        assert(ret == ESP_OK);
        if (spi_hid_buffer->header != HEADER_HID_TRANSMISSION ||
            spi_hid_buffer->crc != esp_crc16_le(UINT16_MAX, (void*)&spi_hid_buffer->key_event, sizeof(hid_keyboard_report_t))){
            ESP_LOGI(pcTaskGetName(NULL), "SPI received transmission invalid with => %x; %x;", spi_hid_buffer->header, spi_hid_buffer->crc);

            // Where are not expecting an invalid SPI transmission.
            // But this happens when the other device is turned off.
            // Disabling SPI for 500ms agains incorrect transaction.
            spi_slave_disable(SPI_HID_RECEIVER);
            vTaskDelay(pdMS_TO_TICKS(500));
            spi_slave_enable(SPI_HID_RECEIVER);
            continue;
        }
        #if DEBUG_LOG
        ESP_LOGI(pcTaskGetName(NULL), "SPI received transmission of type => %x", spi_hid_buffer->header);
        #endif

        // Process keyboard report
        //// Pre-hook keyboard USB transmission
        macro_prehook_transmission(&spi_hid_buffer->key_event);
        //// Add to queue to be report to USB device
        hid_add_keyboard_report(spi_hid_buffer->key_event);
        //// Post-hook keyboard USB transmission
        macro_posthook_transmission(&spi_hid_buffer->key_event);
    }
}

// Init SPI slave for HID report
void spi_init_slave_hid_receiver(){
    //Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    //Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = GPIO_CS,
        .queue_size = 3,
        .flags = 0,
        .post_setup_cb = NULL,
        .post_trans_cb = NULL
    };

    //Initialize SPI slave interface
    esp_err_t ret = spi_slave_initialize(SPI_HID_RECEIVER, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);

    spi_hid_buffer = spi_bus_dma_memory_alloc(SPI_HID_RECEIVER, sizeof(spi_hid_transmit_t), 0);
    assert(spi_hid_buffer);

    memset(&spi_transaction_hid_receiver, 0, sizeof(spi_slave_transaction_t));

    // Create the consumer task (priority = 23)
    xTaskCreatePinnedToCore(spi_task_slave_hid_receiver, "SPI HID Receiver", 4096, NULL, 23, NULL, 0);
}

// Init SPI host for PC report
void spi_init_master_pc_sender(){
    //Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI2,
        .miso_io_num = GPIO_MISO2,
        .sclk_io_num = GPIO_SCLK2,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    //Configuration for the SPI device on the other side of the bus
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .clock_speed_hz = 80 * 1000 * 1000, //Clock out at 80 MHz
        .duty_cycle_pos = 128,      //50% duty cycle
        .mode = 0,
        .spics_io_num = GPIO_CS2,
        .cs_ena_posttrans = 3,      //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .queue_size = 3
    };

    //Initialize the SPI bus and add the device we want to send stuff to.
    esp_err_t ret = spi_bus_initialize(SPI_PC_SENDER, &buscfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);
    ret = spi_bus_add_device(SPI_PC_SENDER, &devcfg, &spi_handle_pc_sender);
    assert(ret == ESP_OK);

    spi_pc_buffer = spi_bus_dma_memory_alloc(SPI_PC_SENDER, sizeof(spi_pc_transmit_t), 0);
    assert(spi_pc_buffer);

    memset(&spi_transaction_pc_sender, 0, sizeof(spi_transaction_t));
}

// Send PC report over SPI
void spi_send_master_pc_sender(uint8_t const* buffer){
    // Set buffer information
    spi_pc_buffer->header = HEADER_PC_TRANSMISSION;
    memcpy((void*)&spi_pc_buffer->led, buffer, sizeof(uint8_t));
    spi_pc_buffer->crc = esp_crc16_le(UINT16_MAX, (void*)&spi_pc_buffer->led, sizeof(char));

    // Prepare transmission
    memset(&spi_transaction_pc_sender, 0, sizeof(spi_transaction_pc_sender));
    spi_transaction_pc_sender.length = sizeof(spi_pc_transmit_t) * 8;
    spi_transaction_pc_sender.tx_buffer = spi_pc_buffer;
    spi_transaction_pc_sender.rx_buffer = NULL;

    // Transmis data
    #if DEBUG_LOG
    ESP_LOGI(pcTaskGetName(NULL), "SPI send transmission of type => %x", spi_pc_buffer->header);
    #endif
    esp_err_t ret = spi_device_transmit(spi_handle_pc_sender, &spi_transaction_pc_sender);
    assert(ret == ESP_OK);
}
