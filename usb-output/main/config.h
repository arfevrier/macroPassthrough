// External project dependencies
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_slave.h"
#include "esp_crc.h"
#include "esp_timer.h"

// Local dependencies
#include "macpass_macro.h"
#include "macpass_spi.h"
#include "macpass_hid.h"
#include "macpass_usb.h"
#include "macpass_tool.h"

// Define title for logging: for UART debug purposes
#define LOG_TITLE "MacroPassthrough"

// Enable debug log, (it impacts performance)
#define DEBUG_LOG 0

// ---
// --- TinyUSB descriptors
// ---

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)
static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
};
static const char* hid_string_descriptor[5] = {
    (char[]){0x09, 0x04},     // 0: is supported language is English (0x0409)
    "TinyUSB",                // 1: Manufacturer
    "TinyUSB Device",         // 2: Product
    "123456",                 // 3: Serials, should use chip ID
    "Example HID interface",  // 4: HID
};
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, true, sizeof(hid_report_descriptor), 0x81, 16, 1),
};

// ---
// USER sequence
// ---

static const group_sequence_t macro_sequence = {
    .list = {
        {
            .list = {
                {1000*1000, 0},
                {1000*1000, HID_KEY_KEYPAD_2},
                {0, HID_KEY_KEYPAD_6},
                {10*1000, 0},
                {1000*1000, HID_KEY_KEYPAD_2},
                {10*1000, 0},
            },
            .size = 6,
            .detect_key = HID_KEY_KEYPAD_3,
        },
        {
            .list = {
                {0, 0},
                {1000*1000, HID_KEY_A},
                {0, HID_KEY_B},
                {2000*1000, 0},
                {1000*1000, HID_KEY_A},
                {10*1000, 0},
            },
            .size = 6,
            .detect_key = HID_KEY_KEYPAD_3,
        },
        {
            .list = {
                {0, HID_KEY_A},
                {150*1000, 0},
                {0, HID_KEY_D},
                {150*1000, 0},
            },
            .size = 4,
            .loop = true,
            .detect_key = HID_KEY_X,
        },
    }
};
