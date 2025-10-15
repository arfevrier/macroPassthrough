// Import global project config
#include "config.h"

static const char *hid_proto_name_str[] = {
    "NONE",
    "KEYBOARD",
    "MOUSE"
};

hid_host_device_handle_t usb_keyboard_handle = NULL;
QueueHandle_t hid_event_queue = NULL;
#if DEBUG_LOG
int64_t report_time;
#endif

/**
 * @brief USB HID Host interface callback
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host interface event
 * @param[in] arg                Pointer to arguments, does not used
 */
void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle, const hid_host_interface_event_t event, void *arg){
    uint8_t data[32] = {0};
    size_t data_length = 0;
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));
    switch (event) {
        case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
            ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(hid_device_handle, data, sizeof(data), &data_length));
            #if DEBUG_LOG
            ESP_LOGI(LOG_TITLE, "HID Report subclass: %d, proto %d, size: %d", dev_params.sub_class, dev_params.proto, data_length);
            #endif

            // Manage different type of report
            #if DEBUG_LOG
            if (data_length==0) break;
            #endif
            if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
                // Keyboard report
                if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                    #if DEBUG_LOG
                    if (keycode_contains_key(*((hid_keyboard_report_t*)data), HID_KEY_CAPS_LOCK)){
                        report_time = esp_timer_get_time();
                    }
                    #endif
                    spi_send_master_hid_sender(HEADER_HID_KEYBOARD_TRANSMISSION, (hid_report_t*)data);
                // Mouse report
                } else if (HID_PROTOCOL_MOUSE == dev_params.proto) {
                    spi_send_master_hid_sender(HEADER_HID_MOUSE_TRANSMISSION, (hid_report_t*)data);
                }
            }
            break;
        case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
            ESP_LOGI(LOG_TITLE, "HID Device, protocol '%s' DISCONNECTED", hid_proto_name_str[dev_params.proto]);
            ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
            break;
        case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
            ESP_LOGI(LOG_TITLE, "HID Device, protocol '%s' TRANSFER_ERROR", hid_proto_name_str[dev_params.proto]);
            break;
        default:
            ESP_LOGE(LOG_TITLE, "HID Device, protocol '%s' Unhandled event", hid_proto_name_str[dev_params.proto]);
            break;
    }
}

/**
 * @brief USB HID Host Device event
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host Device event
 * @param[in] arg                Pointer to arguments, does not used
 */
void hid_host_device_event(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event, void *arg){
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_DRIVER_EVENT_CONNECTED:
        ESP_LOGI(LOG_TITLE, "HID Device, protocol '%s' CONNECTED", hid_proto_name_str[dev_params.proto]);
        
        // ESP32-S3 only have few USB handle available.
        //  we need to skip NONE peripheral to connect in order
        //  to works with USB hub. Because gaming Keyboard/Mouse 
        //  can have 4-5 HID device per peripheral.
        if(dev_params.proto == 0 || dev_params.proto > 2){
            ESP_LOGI(LOG_TITLE, "HID Device, skipped");
            break;
        }

        const hid_host_device_config_t dev_config = {
            .callback = hid_host_interface_callback,
            .callback_arg = NULL
        };

        ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
        #if DEBUG_LOG
        ESP_LOGI(LOG_TITLE, "DEBUG: handle=%p, sub_class=%d, proto=%d", hid_device_handle, dev_params.sub_class, dev_params.proto);
        #endif
        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            #if DEBUG_LOG
            ESP_LOGI(LOG_TITLE, "DEBUG: Attempting to set BOOT protocol");
            #endif
            esp_err_t proto_ret = hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT);
            if (proto_ret != ESP_OK) {
                ESP_LOGE(LOG_TITLE, "Set BOOT protocol failed: %s", esp_err_to_name(proto_ret));
            }
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                usb_keyboard_handle = hid_device_handle;
                ESP_ERROR_CHECK(hid_class_request_set_idle(hid_device_handle, 0, 0));
            }
        }
        ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));
        break;
    default:
        break;
    }
}

/**
 * @brief HID Host Device callback
 *
 * Puts new HID Device event to the queue
 *
 * @param[in] hid_device_handle HID Device handle
 * @param[in] event             HID Device event
 * @param[in] arg               Not used
 */
void hid_host_device_callback(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event, void *arg){
    static hid_event_queue_t evt_queue;
    evt_queue.handle = hid_device_handle;
    evt_queue.event = event;
    evt_queue.arg = arg;

    xQueueSend(hid_event_queue, &evt_queue, 0);
}

void hid_host_keyboard_report_output(char report){
    if (usb_keyboard_handle != NULL) {
        #if DEBUG_LOG
        ESP_LOGI(pcTaskGetName(NULL), "CapsLock ping => %lld us", esp_timer_get_time()-report_time);
        #endif
        esp_err_t ret = hid_class_request_set_report(usb_keyboard_handle, HID_REPORT_TYPE_OUTPUT, 0, (void*)&report, sizeof(char));
        assert(ret == ESP_OK);
    }
}

/**
 * @brief Start USB Host install and handle common USB host library events
 *
 * @param[in] arg  Not used
 */
void usb_lib_task(void *arg){
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LOWMED,
    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));

    const hid_host_driver_config_t hid_host_driver_config = {
        .create_background_task = true,
        .task_priority = USB_TASK_PRIORITY,
        .stack_size = 4096,
        .core_id = USB_TASK_COREID,
        .callback = hid_host_device_callback,
        .callback_arg = NULL
    };

    ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

    while (true) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        // In this example, there is only one client registered
        // So, once we deregister the client, this call must succeed with ESP_OK
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_ERROR_CHECK(usb_host_device_free_all());
            break;
        }
    }

    ESP_LOGI(LOG_TITLE, "USB shutdown");
    // Clean up USB Host
    vTaskDelay(10); // Short delay to allow clients clean-up
    ESP_ERROR_CHECK(usb_host_uninstall());
    vTaskDelete(NULL);
}

void hid_lib_task(void *arg){
    hid_event_queue_t evt_queue;

    while (true) {
        if (xQueueReceive(hid_event_queue, &evt_queue, portMAX_DELAY)) {
            hid_host_device_event(evt_queue.handle, evt_queue.event, evt_queue.arg);
        }
    }
}

void usb_init(void){
    /*
    * Create usb_lib_task to:
    * - initialize USB Host library
    * - Handle USB Host events while APP pin in in HIGH state
    */
    hid_event_queue = xQueueCreate(10, sizeof(hid_event_queue_t));
    xTaskCreatePinnedToCore(usb_lib_task, "usb_events", 4096, NULL, USB_TASK_PRIORITY, NULL, USB_TASK_COREID);
    xTaskCreatePinnedToCore(hid_lib_task, "hid_events", 4096, NULL, USB_TASK_PRIORITY, NULL, USB_TASK_COREID);
}
