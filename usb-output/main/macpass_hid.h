
#define HEADER_HID_TRANSMISSION 0xA1
#define HEADER_PC_TRANSMISSION 0xA2

void hid_init_multiplexer();
void hid_add_keyboard_report(hid_keyboard_report_t report);
