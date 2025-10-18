
// Define size of sequence structure. Set as lower as possible. Can impact performance.
#define HISTORY_SIZE 2
#define MAX_KEY_MODIFICATION_EVENT 30
#define MAX_KEY_MODIFICATION_SEQUENCE 10

typedef struct {
    unsigned int duration;
    hid_transmit_t event;
} key_modification_event_t;

typedef struct {
    // --- User defined variable
    key_modification_event_t list[MAX_KEY_MODIFICATION_EVENT]; // List of HID event to send
    uint8_t size; // Size of the list of event
    hid_transmit_t event_press; // Detect on press 
    hid_transmit_t event_release; // Detect on release
    bool loop; // Play the sequence on a loop

    // --- Computed
    hid_transmit_t previous_key;
    uint8_t pos;
    esp_timer_handle_t timer;
    esp_timer_create_args_t timer_args;
} key_modification_sequence_t;

typedef struct {
    key_modification_sequence_t list[MAX_KEY_MODIFICATION_SEQUENCE];
} group_sequence_t;

bool macro_prehook_transmission(hid_transmit_t* report);
void macro_posthook_transmission(hid_transmit_t* report);
void macro_sequence_callback(void* arg);
void macro_init();
