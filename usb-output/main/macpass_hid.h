
#define HEADER_HID_KEYBOARD_TRANSMISSION 0xA1
#define HEADER_HID_MOUSE_TRANSMISSION 0xA3
#define HEADER_PC_TRANSMISSION 0xA2

void hid_init_multiplexer();
void hid_add_report(hid_transmit_t report);
