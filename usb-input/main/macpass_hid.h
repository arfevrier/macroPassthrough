
#define HEADER_HID_KEYBOARD_TRANSMISSION 0xA1
#define HEADER_HID_MOUSE_TRANSMISSION 0xA3
#define HEADER_PC_TRANSMISSION 0xA2

typedef struct {
  uint8_t modifier;   /**< Keyboard modifier (KEYBOARD_MODIFIER_* masks). */
  uint8_t reserved;   /**< Reserved for OEM use, always set to 0. */
  uint8_t keycode[6]; /**< Key codes of the currently pressed keys. */
} hid_keyboard_report_t;

typedef struct
{
  uint8_t buttons; /**< buttons mask for currently pressed buttons in the mouse. */
  int8_t  x;       /**< Current delta x movement of the mouse. */
  int8_t  y;       /**< Current delta y movement on the mouse. */
  int8_t  wheel;   /**< Current delta wheel movement on the mouse. */
  int8_t  pan;     // using AC Pan
} hid_mouse_report_t;

typedef union {
    hid_keyboard_report_t keyboard;
    hid_mouse_report_t mouse;
} hid_report_t;
