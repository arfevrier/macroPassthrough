
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"

#define BYTE_TO_BINARY(byte)  \
((byte) & 0x80 ? '1' : '0'), \
((byte) & 0x40 ? '1' : '0'), \
((byte) & 0x20 ? '1' : '0'), \
((byte) & 0x10 ? '1' : '0'), \
((byte) & 0x08 ? '1' : '0'), \
((byte) & 0x04 ? '1' : '0'), \
((byte) & 0x02 ? '1' : '0'), \
((byte) & 0x01 ? '1' : '0')

#define ONE_KEYBOARD_KEY(key) \
{ \
    .header = HEADER_HID_KEYBOARD, \
    .event = {.keyboard = {.keycode = {key}}} \
} \

#define TWO_KEYBOARD_KEY(key, key2) \
{ \
    .header = HEADER_HID_KEYBOARD, \
    .event = {.keyboard = {.keycode = {key, key2}}} \
} \

#define EMPTY_KEYBOARD \
{ \
    .header = HEADER_HID_KEYBOARD, \
} \

#define EMPTY_MOUSE \
{ \
    .header = HEADER_HID_MOUSE, \
} \

#define MOUSE_MOUVEMENT(mov_x, mov_y) \
{ \
    .header = HEADER_HID_MOUSE, \
    .event = { \
        .mouse = { \
            .x = mov_x, \
            .y = mov_y, \
        } \
    } \
 }\

static inline void print_keyboard_report(const char* title, const hid_keyboard_report_t report){
    char line[128];
    int offset = snprintf(line, sizeof(line), "Keyboard report [ ");
    for (int i = 0; i < sizeof(hid_keyboard_report_t); i++) {
        if (i == 0) {
            offset += snprintf(line + offset, sizeof(line) - offset, ""BYTE_TO_BINARY_PATTERN"; ", BYTE_TO_BINARY(((char*)&report)[i]));
        } else if (i == 1){
            offset += snprintf(line + offset, sizeof(line) - offset, "%02X; ", ((char*)&report)[i]);
        } else {
            offset += snprintf(line + offset, sizeof(line) - offset, "%02X ", ((char*)&report)[i]);
        }
    }
    offset += snprintf(line + offset, sizeof(line) - offset, "]");
    ESP_LOGI(title, "%s", line);
};

/**
 * Displays the contents of a HID mouse report.
 */
static inline void print_mouse_report(const char* title, const hid_mouse_report_t report){
    char line[128];
    int offset = snprintf(line, sizeof(line), "Mouse report [ buttons: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(report.buttons));
    offset += snprintf(line + offset, sizeof(line) - offset, "; x: %d; y: %d; wheel: %d; pan: %d ]",
        report.x, report.y, report.wheel, report.pan);
    ESP_LOGI(title, "%s", line);
}

static inline bool keycode_contains_key(const hid_keyboard_report_t report, const uint8_t keycode){
    for (int i = 0; i < 6; i++){
        uint8_t key = report.keycode[i];
        if (key == 0) break;
        if (key == keycode){
            return true;
        }
    }
    return false;
}

static inline void add_keycode(hid_keyboard_report_t* report, const uint8_t keycode){
    for (int i = 0; i < 6; i++){
        if (report->keycode[i] == 0){
            report->keycode[i] = keycode;
            break;
        }
    }
};

static inline void remove_keycode(hid_keyboard_report_t* report, const uint8_t keycode){
    for (int i = 0; i < 6; i++){
        if (report->keycode[i] == keycode || report->keycode[i] == 0){
            report->keycode[i] = 0;
            break;
        }
    }
};

static inline bool keyboard_report_contains_event(const hid_keyboard_report_t report, const hid_keyboard_report_t expected){
    // Check modifier keys
    if (expected.modifier != 0) {
        // All bits set in expected must also be set in report
        if ((report.modifier & expected.modifier) != expected.modifier) {
            return false;
        }
    }
    // Check keycodes
    for (int i = 0; i < 6; i++) {
        uint8_t key = expected.keycode[i];
        if (key == 0) break;
        if (!keycode_contains_key(report, key)) {
            return false;
        }
    }
    return true;
}
static inline bool mouse_report_contains_event(const hid_mouse_report_t report, const hid_mouse_report_t expected){
    // Assumption: standard structure
    if (expected.buttons != 0){
        if ((report.buttons & expected.buttons) != expected.buttons){
            return false;
        }
    }  
    return true;
}

/**
 * Adds the contents of src into dst for HID reports.
 * - For keyboard: adds modifiers and merges keycodes (avoids duplicates).
 * - For mouse: only adds buttons (bitwise OR), ignores x/y/wheel/pan.
 * - If headers are different, does nothing.
 */
static inline void add_event_to_report(hid_transmit_t* dst, const hid_transmit_t src) {
    if (dst->header != src.header) return;
    if (dst->header == HEADER_HID_KEYBOARD) {
        dst->event.keyboard.modifier |= src.event.keyboard.modifier;
        for (int i = 0; i < 6; i++) {
            uint8_t key = src.event.keyboard.keycode[i];
            if (key == 0) break;
            add_keycode(&dst->event.keyboard, key);
        }
    } else if (dst->header == HEADER_HID_MOUSE) {
        dst->event.mouse.buttons |= src.event.mouse.buttons;
        dst->event.mouse.pan |= src.event.mouse.pan;
    }
}

// Set mouse movement from src to dst for HID mouse reports.
static inline void set_mouse_movement_to_report(hid_mouse_report_t* dst, const hid_mouse_report_t src) {
    dst->x = src.x;
    dst->y = src.y;
    dst->wheel = src.wheel;
}
