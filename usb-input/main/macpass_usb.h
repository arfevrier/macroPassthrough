
#define USB_TASK_PRIORITY 23
#define USB_TASK_COREID 1
typedef struct {
    hid_host_device_handle_t handle;
    hid_host_driver_event_t event;
    void *arg;
} hid_event_queue_t;

void usb_init(void);
void hid_host_keyboard_report_output(char report);
