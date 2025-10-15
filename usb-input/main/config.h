// External project dependencies
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "usb/usb_host.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/hid_usage_mouse.h"
#include "errno.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_slave.h"
#include "esp_crc.h"
#include "esp_timer.h"

// Local dependencies
#include "macpass_hid.h"
#include "macpass_spi.h"
#include "macpass_usb.h"
#include "../../usb-output/main/macpass_tool.h"

// Define title for logging: for UART debug purposes
#define LOG_TITLE "MacroPassthrough"

// Enable debug log, (it impacts performance)
#define DEBUG_LOG 0
