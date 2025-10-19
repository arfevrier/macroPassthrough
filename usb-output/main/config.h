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
#include "macpass_spi.h"
#include "macpass_hid.h"
#include "macpass_usb.h"
#include "macpass_macro.h"
#include "macpass_tool.h"

// Define title for logging: for UART debug purposes
#define LOG_TITLE "MacroPassthrough"

// Enable debug log, (it impacts performance)
#define DEBUG_LOG 0

// ---
// --- TinyUSB descriptors
// ---

#define CUSTOM_CONFIG 0
#if CUSTOM_CONFIG
#include "config_custom.h"
#else
#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)
static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
};
static const char* hid_string_descriptor[5] __unused = {
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
static const tusb_desc_device_t espressif_device = {
    .bLength            = 0x12,
    .bDescriptorType    = 0x01,
    .bcdUSB             = 0x200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 0x40,
    .idVendor           = 0x303A,
    .idProduct          = 0x4004,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// ---
// USER sequence
// ---

static const group_sequence_t macro_sequence = {
    .list = {
        {
            .list = {
                {1000*1000, EMPTY_KEYBOARD},
                {1000*1000, ONE_KEYBOARD_KEY(HID_KEY_KEYPAD_2)},
                {0, ONE_KEYBOARD_KEY(HID_KEY_KEYPAD_6)},
                {10*1000, EMPTY_KEYBOARD},
                {1000*1000, ONE_KEYBOARD_KEY(HID_KEY_KEYPAD_2)},
                {10*1000, EMPTY_KEYBOARD},
            },
            .size = 6,
            .event_press = ONE_KEYBOARD_KEY(HID_KEY_KEYPAD_3),
        },
        {
            .list = {
                {0, EMPTY_KEYBOARD},
                {1000*1000, ONE_KEYBOARD_KEY(HID_KEY_A)},
                {0, ONE_KEYBOARD_KEY(HID_KEY_B)},
                {2000*1000, EMPTY_KEYBOARD},
                {1000*1000, ONE_KEYBOARD_KEY(HID_KEY_A)},
                {10*1000, EMPTY_KEYBOARD},
            },
            .size = 6,
            .event_press = ONE_KEYBOARD_KEY(HID_KEY_KEYPAD_3),
        },
        {
            .list = {
                {0, ONE_KEYBOARD_KEY(HID_KEY_A)},
                {150*1000, EMPTY_KEYBOARD},
                {0, ONE_KEYBOARD_KEY(HID_KEY_D)},
                {150*1000, EMPTY_KEYBOARD},
            },
            .size = 4,
            .loop = true,
            .event_press = ONE_KEYBOARD_KEY(HID_KEY_X),
        },
        {
            .list = {
                {3*1000, MOUSE_MOUVEMENT(-2, 0)},
            },
            .size = 1,
            .loop = true,
            .event_press = ONE_KEYBOARD_KEY(HID_KEY_ARROW_LEFT),
        },
        {
            .list = {
                {3*1000, MOUSE_MOUVEMENT(2, 0)},
            },
            .size = 1,
            .loop = true,
            .event_press = ONE_KEYBOARD_KEY(HID_KEY_ARROW_RIGHT),
        },
        {
            .list = {
                {3*1000, MOUSE_MOUVEMENT(0, -2)},
            },
            .size = 1,
            .loop = true,
            .event_press = ONE_KEYBOARD_KEY(HID_KEY_ARROW_UP),
        },
        {
            .list = {
                {3*1000, MOUSE_MOUVEMENT(0, 2)},
            },
            .size = 1,
            .loop = true,
            .event_press = ONE_KEYBOARD_KEY(HID_KEY_ARROW_DOWN),
        },
        {
            .list = {
                {50*1000, MOUSE_MOUVEMENT(-10, 10)},
            },
            .size = 29,
            .loop = true,
            .event_press = ONE_MOUSE_KEY(MOUSE_BUTTON_RIGHT),
        },
        {
            .list = {
                {50000, ONE_KEYBOARD_KEY(HID_KEY_A)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_B)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_C)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_D)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_E)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_F)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_G)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_H)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_I)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_J)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_K)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_L)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_M)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_N)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_O)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_P)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_Q)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_R)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_S)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_T)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_U)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_V)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_W)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_X)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_Y)},
                {50000, ONE_KEYBOARD_KEY(HID_KEY_Z)},
                {0, EMPTY_KEYBOARD}
            },
            .size = 27,
            .event_press = ONE_MOUSE_KEY(MOUSE_BUTTON_RIGHT),
        },
        {
            .size = 1,
            .save_press = ONE_KEYBOARD_KEY(HID_KEY_KEYPAD_SUBTRACT),
            .event_press = ONE_KEYBOARD_KEY(HID_KEY_KEYPAD_ADD),
        },
    }
};
#endif
