
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

static inline void print_keyboard_report(const char* title, hid_keyboard_report_t report){
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

static inline bool keycode_contains_key(hid_keyboard_report_t report, uint8_t keycode){
    for (int i = 0; i < 6; i++){
        if (report.keycode[i] == keycode){
            return true;
        }
    }
    return false;
};

static inline void add_keycode(hid_keyboard_report_t* report, uint8_t keycode){
    for (int i = 0; i < 6; i++){
        if (report->keycode[i] == 0){
            report->keycode[i] = keycode;
            break;
        }
    }
    return;
};

static inline void remove_keycode(hid_keyboard_report_t* report, uint8_t keycode){
    for (int i = 0; i < 6; i++){
        if (report->keycode[i] == keycode || report->keycode[i] == 0){
            report->keycode[i] = 0;
            break;
        }
    }
    return;
};