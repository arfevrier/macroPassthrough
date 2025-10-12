// Import global project config
#include "config.h"

spi_transaction_t spi_transaction_hid_sender;
spi_hid_transmit_t* spi_hid_buffer;
spi_device_handle_t spi_handle_hid_sender;

spi_slave_transaction_t spi_transaction_pc_receiver;
spi_pc_transmit_t* spi_pc_buffer;

// Init SPI master for HID report
void spi_init_master_hid_sender(){
    //Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
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
        .spics_io_num = GPIO_CS,
        .cs_ena_posttrans = 3,      //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .queue_size = 3
    };

    //Initialize the SPI bus and add the device we want to send stuff to.
    esp_err_t ret = spi_bus_initialize(SPI_HID_SENDER, &buscfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);

    ret = spi_bus_add_device(SPI_HID_SENDER, &devcfg, &spi_handle_hid_sender);
    assert(ret == ESP_OK);

    spi_hid_buffer = spi_bus_dma_memory_alloc(SPI_HID_SENDER, sizeof(spi_hid_transmit_t), 0);
    assert(spi_hid_buffer);   
}

// Init SPI slave for PC receive
void spi_init_slave_pc_receiver(){
    //Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI2,
        .miso_io_num = GPIO_MISO2,
        .sclk_io_num = GPIO_SCLK2,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    //Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = GPIO_CS2,
        .queue_size = 3,
        .flags = 0,
        .post_setup_cb = NULL,
        .post_trans_cb = NULL,
    };

    //Initialize SPI slave interface
    esp_err_t ret = spi_slave_initialize(SPI_PC_RECEIVER, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);

    spi_pc_buffer = spi_bus_dma_memory_alloc(SPI_PC_RECEIVER, sizeof(spi_pc_transmit_t), 0);
    assert(spi_pc_buffer);

    // Create the consumer task (priority = 23)
    xTaskCreatePinnedToCore(spi_task_slave_pc_receiver, "spi_pc_receiver", 2048, NULL, 23, NULL, 0);
}

// Task to handle SPI data reception from the PC port
void spi_task_slave_pc_receiver(void *pvParameters){
    while(true){
        // Clear the buffer for new transaction
        memset(spi_pc_buffer, 0, sizeof(spi_pc_transmit_t));
        spi_transaction_pc_receiver.length = sizeof(spi_pc_transmit_t) * 8;
        spi_transaction_pc_receiver.tx_buffer = NULL;
        spi_transaction_pc_receiver.rx_buffer = spi_pc_buffer;

        // Wait for the next SPI transmission
        esp_err_t ret = spi_slave_transmit(SPI_PC_RECEIVER, &spi_transaction_pc_receiver, portMAX_DELAY);
        
        // Validate the received buffer
        assert(ret == ESP_OK);
        if (spi_pc_buffer->header != HEADER_PC_TRANSMISSION ||
            spi_pc_buffer->crc != esp_crc16_le(UINT16_MAX, (void*)&spi_pc_buffer->led, sizeof(char))){
            ESP_LOGI(pcTaskGetName(NULL), "SPI received transmission invalid with => %x; %x;", spi_pc_buffer->header, spi_pc_buffer->crc);
            
            // Where are not expecting an invalid SPI transmission.
            // But this happens when the other device is turned off.
            // Disabling SPI for 500ms agains incorrect transaction.
            spi_slave_disable(SPI_PC_RECEIVER);
            vTaskDelay(pdMS_TO_TICKS(500));
            spi_slave_enable(SPI_PC_RECEIVER);
            continue;
        }
        #if DEBUG_LOG
        ESP_LOGI(pcTaskGetName(NULL), "SPI received transmission of type => %x", spi_hid_buffer->header);
        #endif

        // Report to USB HID device the PC output report
        hid_host_keyboard_report_output(spi_pc_buffer->led);
    }
}

void spi_send_master_hid_sender(hid_keyboard_report_t* report){
    // Set buffer information
    spi_hid_buffer->header = HEADER_HID_TRANSMISSION;
    memcpy((void*)&spi_hid_buffer->key_event, (void*)report, sizeof(hid_keyboard_report_t));
    spi_hid_buffer->crc = esp_crc16_le(UINT16_MAX, (void*)&spi_hid_buffer->key_event, sizeof(hid_keyboard_report_t));

    // Prepare transmission
    spi_transaction_hid_sender.length = sizeof(spi_hid_transmit_t) * 8;
    spi_transaction_hid_sender.tx_buffer = spi_hid_buffer;
    spi_transaction_hid_sender.rx_buffer = NULL;

    // Transmis data
    esp_err_t ret = spi_device_transmit(spi_handle_hid_sender, &spi_transaction_hid_sender);
    assert(ret == ESP_OK);
    #if DEBUG_LOG
    ESP_LOGI(pcTaskGetName(NULL), "HID transmission done");
    #endif
}

void spi_uninstall(){
    esp_err_t ret = spi_bus_remove_device(spi_handle_hid_sender);
    assert(ret == ESP_OK);
}
