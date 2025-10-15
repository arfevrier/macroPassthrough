// Import global project config
#include "config.h"

// --- TinyUSB HID callbacks

// Invoked when received GET HID REPORT DESCRIPTOR request
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance){
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen){
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize){
    if (report_type == HID_REPORT_TYPE_OUTPUT) {
        spi_send_master_pc_sender(buffer);
    }
}

// Initialisation function
void tud_user_initialization(){
    ESP_LOGI(LOG_TITLE, "USB initialization");
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = NULL;
    tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;
    tusb_cfg.descriptor.string = hid_string_descriptor;
    tusb_cfg.descriptor.string_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]);
    tusb_cfg.task.xCoreID = 1;
    tusb_cfg.task.priority = 23;
#if (TUD_OPT_HIGH_SPEED)
    tusb_cfg.descriptor.high_speed_config = hid_configuration_descriptor;
#endif // TUD_OPT_HIGH_SPEED
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(LOG_TITLE, "USB initialization DONE");
}
