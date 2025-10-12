
// Define size of sequence structure. Set as lower as possible. Can impact performance.
#define MAX_KEY_MODIFICATION_EVENT 25
#define MAX_KEY_MODIFICATION_SEQUENCE 10

typedef struct {
    unsigned int duration;
    uint8_t key;
} key_modification_event_t;

typedef struct {
    key_modification_event_t list[MAX_KEY_MODIFICATION_EVENT];
    uint8_t detect_key;
    uint8_t detect_unkey;
    uint8_t previous_key;
    uint8_t size;
    uint8_t pos;
    bool loop;
    esp_timer_handle_t timer;
    esp_timer_create_args_t timer_args;
} key_modification_sequence_t;

typedef struct {
    key_modification_sequence_t list[MAX_KEY_MODIFICATION_SEQUENCE];
} group_sequence_t;

void macro_prehook_transmission(hid_keyboard_report_t* report);
void macro_posthook_transmission(hid_keyboard_report_t* report);
void macro_sequence_callback(void* arg);
void macro_init();
