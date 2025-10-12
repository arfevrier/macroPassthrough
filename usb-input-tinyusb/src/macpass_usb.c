
// Import global project config
#include "config.h"

static uint8_t g_keyboard_dev_addr = 0;
static uint8_t g_keyboard_instance = 0;

// Invoked when device is mounted (configured)
void tuh_mount_cb(uint8_t daddr) {
  printf("Device %u\n", daddr);
}

// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr) {
  printf("Device removed, address = %d\r\n", daddr);
}

// Callback when HID device is mounted
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  (void)desc_report;
  (void)desc_len;
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
  printf("VID = %04x, PID = %04x\r\n", vid, pid);

  // Store keyboard device address and instance
  g_keyboard_dev_addr = dev_addr;
  g_keyboard_instance = instance;

  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    printf("Error: cannot request to receive report\r\n");
  }
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
  //printf("len = %d\n", len);
  spi_send_master_hid_sender((hid_keyboard_report_t*)report);

  // continue to request to receive report
  if (!tuh_hid_receive_report(dev_addr, instance)) {
    printf("Error: cannot request to receive report\r\n");
  }
}

void hid_host_keyboard_report_output(uint8_t led_state) {
  if (g_keyboard_dev_addr && tuh_ready(g_keyboard_dev_addr)) {
    tuh_hid_set_report(g_keyboard_dev_addr, g_keyboard_instance, HID_REPORT_TYPE_OUTPUT, 0, &led_state, 1);
  }
}

// Run TinyUSB main loop
void usb_host_task(void *param) {
  // Run main TinyUSB loop
  while (true) {
    tuh_task();
    // !!
    // Some USB Hub doesn't works
    // Without adding an extra delay
    // Setting to 1, should not impact the USB pooling rate too much...
    // !!
    vTaskDelay(1);
  }
}

void usb_initialization(void) {
  // Init TinyUSB board dependency
  board_init();

  // Init TinyUSB lib
  tusb_rhport_init_t host_init = {
    .role = TUSB_ROLE_HOST,
    .speed = TUSB_SPEED_AUTO
  };
  if (!tusb_init(BOARD_TUH_RHPORT, &host_init)) {
    printf("Failed to init USB Host Stack\r\n");
    vTaskSuspend(NULL);
  }

  // Finish init TinyUSB board dependency
  board_init_after_tusb();

  // Run TinyUSB main loop
  xTaskCreatePinnedToCore(usb_host_task, "usbh", 4096, NULL, 23, NULL, 1);
}
