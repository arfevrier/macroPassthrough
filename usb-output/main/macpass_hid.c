// Import global project config
#include "config.h"

// Queue to store HID reports
QueueHandle_t global_hid_report_queue = NULL;
// Create a semaphore to manage HID sending timing
// > Performance note: xTaskGetCurrentTaskHandle is less demanding than a new xSemaphoreCreateBinary. 
TaskHandle_t hid_task_wait_somaphore = NULL;

// Callback to release the HID task semaphore
void hid_task_multiplexer_release_cb(void *arg){
    xTaskNotifyGive(hid_task_wait_somaphore);
}

// Consumer task: send all reports from queue to USB PC
void hid_task_multiplexer(void *pvParameters) {
  // Initialize the task semaphore
  hid_task_wait_somaphore = xTaskGetCurrentTaskHandle();
  // One notify to avoid blocking at first iteration
  xTaskNotifyGive(hid_task_wait_somaphore); 

  // Create a hardware timer to manage high precision lock
  esp_timer_create_args_t timer_args = {
    .callback = &hid_task_multiplexer_release_cb,
    .name = "timer_hid_lock"
  };
  esp_timer_handle_t timer;
  esp_timer_create(&timer_args, &timer);

  // Consumer loop
  while (true) {
    hid_transmit_t queue_received;
    if (xQueueReceive(global_hid_report_queue, &queue_received, portMAX_DELAY) == pdTRUE) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait until timer release the lock
      esp_timer_start_once(timer, 1000-100); // 1ms timeout to avoid blocking if USB is not ready

      // Display in console log
      #if DEBUG_LOG
      if (queue_received.header == HEADER_HID_KEYBOARD_TRANSMISSION){
        print_keyboard_report(pcTaskGetName(NULL), queue_received.event.keyboard);
        ESP_LOGI(pcTaskGetName(NULL), "tud status: %d", tud_ready());
      }
      #endif
      
      // But USB peripheral must still be ready. If the USB device is disconnected, tud_ready() will return false.
      if (!tud_ready()) continue;
      // If the report is not sent, requeue-it.
      static bool status;
      if (queue_received.header == HEADER_HID_KEYBOARD_TRANSMISSION) {
        status = tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, queue_received.event.keyboard.modifier, queue_received.event.keyboard.keycode);
      } else if (queue_received.header == HEADER_HID_MOUSE_TRANSMISSION) {
        status = tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, queue_received.event.mouse.buttons, queue_received.event.mouse.x, queue_received.event.mouse.y, queue_received.event.mouse.wheel, queue_received.event.mouse.pan);
      }
      if (status == false && tud_ready()) {
            ESP_LOGI(pcTaskGetName(NULL), "Sending report to TinyUSB failed => tud status: %d", tud_ready());
            // if USB is not ready, we wait 1ms retry
            xQueueSendToFront(global_hid_report_queue, &queue_received, 0);
      }
    }
  }
}

void hid_init_multiplexer(){
    // Create the queue to hold HID reports
    global_hid_report_queue = xQueueCreate(10, sizeof(hid_transmit_t));

    // Create the consumer task (priority = 23)
    xTaskCreatePinnedToCore(hid_task_multiplexer, "HID Report Multiplexer", 4096, NULL, 23, NULL, 0);
}

void hid_add_report(hid_transmit_t report){
    xQueueSend(global_hid_report_queue, &report, 0);
}
