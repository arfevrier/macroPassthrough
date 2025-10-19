// Import global project config
#include "config.h"

hid_keyboard_report_t last_keyboard_report[HISTORY_SIZE];
hid_mouse_report_t last_mouse_report;
group_sequence_t group_sequence;

void macro_init(){
    // Copy sequence
    group_sequence = macro_sequence;
    // For each macro sequence define by the user
    for (int i = 0; i < MAX_KEY_MODIFICATION_SEQUENCE; i++){
        key_modification_sequence_t* sequence = &group_sequence.list[i];
        // Ignore empty sequence
        if (sequence->size == 0) continue;
        // Set callback
        sequence->timer_args.callback = &macro_sequence_callback;
        sequence->timer_args.arg = &group_sequence.list[i];
        sequence->timer_args.name = malloc(15*sizeof(char));
        snprintf((char*)sequence->timer_args.name, 15, "sequence%d", i);

        // Instanciate an ESP timer
        esp_timer_create(&sequence->timer_args, &sequence->timer);
    }
}

// Prehook for macro HID transmission. Return true to end transmission chain. Default to false.
bool macro_prehook_transmission(hid_transmit_t* report){
    
    // --- START USER CUSTOM MACRO
    if (report->header == HEADER_HID_KEYBOARD){
        if (keycode_contains_key(report->event.keyboard, HID_KEY_A) && keycode_contains_key(report->event.keyboard, HID_KEY_D)){
            if (keycode_contains_key(last_keyboard_report[0], HID_KEY_A)){
                remove_keycode(&report->event.keyboard, HID_KEY_A);
            } else if (keycode_contains_key(last_keyboard_report[0], HID_KEY_D)){
                remove_keycode(&report->event.keyboard, HID_KEY_D);
            }
        }
    }
    // --- END
     
    // Example: Remove a KEY
    // remove_keycode(report, HID_KEY_C);

    // Exemple: Skip all A key
    // if (keycode_contains_key(report->event.keyboard, HID_KEY_A) return true;

    return false;
}

void macro_posthook_transmission(hid_transmit_t* report){

    #if DEBUG_LOG
    ESP_LOGI(pcTaskGetName(NULL), "posthook(): Start function");
    if (report->header == HEADER_HID_KEYBOARD) {
        print_keyboard_report(pcTaskGetName(NULL), report->event.keyboard);
    } else if (report->header == HEADER_HID_MOUSE) {
        print_mouse_report(pcTaskGetName(NULL), report->event.mouse);
    }
    #endif

    // Case n°1: Keyboard HID
    if (report->header == HEADER_HID_KEYBOARD){
        // Update last report transmission, like this all macro are added to real keys press by user
        last_keyboard_report[1] = last_keyboard_report[0];
        last_keyboard_report[0] = report->event.keyboard;

        // --- START USER CUSTOM MACRO
        // --- END

        // Manage sequence start:
        for (int i = 0; i < MAX_KEY_MODIFICATION_SEQUENCE; i++){
            key_modification_sequence_t* sequence = &group_sequence.list[i];
            // Ignore empty sequence
            if (sequence->size == 0) break;
            // If a press key is defined for sequence
            if (sequence->event_press.header == HEADER_HID_KEYBOARD && keyboard_report_contains_event(last_keyboard_report[0], sequence->event_press.event.keyboard)){
                #if DEBUG_LOG
                ESP_LOGI(pcTaskGetName(NULL), "posthook(): Starting keyboard press macro: %s", sequence->timer_args.name);
                #endif
                reset_sequence(sequence);
                start_sequence(sequence);
            }
            // If a unpress key is defined for sequence
            if (sequence->event_release.header == HEADER_HID_KEYBOARD && keyboard_report_contains_event(last_keyboard_report[1], sequence->event_release.event.keyboard) && !keyboard_report_contains_event(last_keyboard_report[0], sequence->event_release.event.keyboard)){
                #if DEBUG_LOG
                ESP_LOGI(pcTaskGetName(NULL), "posthook(): Starting keyboard release macro: %s", sequence->timer_args.name);
                #endif
                reset_sequence(sequence);
                start_sequence(sequence);
            }
            // If a recording sequence
            if (sequence->save_press.header == HEADER_HID_KEYBOARD && sequence->is_recording){
                #if DEBUG_LOG
                ESP_LOGI(pcTaskGetName(NULL), "posthook(): Recording keyboard event");
                #endif
                add_keyboard_record(sequence, *report);
            }
            // If a save press key is defined for sequence
            if (sequence->save_press.header == HEADER_HID_KEYBOARD && keyboard_report_contains_event(last_keyboard_report[0], sequence->save_press.event.keyboard)){
                #if DEBUG_LOG
                ESP_LOGI(pcTaskGetName(NULL), "posthook(): Starting keyboard save macro: %s", sequence->timer_args.name);
                #endif
                reset_sequence(sequence);
                sequence->is_recording = true;
            }
        }
    // Case n°2: Mouse HID
    } else if (report->header == HEADER_HID_MOUSE){
        // Update last report transmission, like this all macro are added to real keys press by user
        last_mouse_report = report->event.mouse;

        // --- START USER CUSTOM MACRO
        // --- END

        // Manage sequence start:
        for (int i = 0; i < MAX_KEY_MODIFICATION_SEQUENCE; i++){
            key_modification_sequence_t* sequence = &group_sequence.list[i];
            // Ignore empty sequence
            if (sequence->size == 0) break;
            // If a press key is defined for sequence
            if (sequence->event_press.header == HEADER_HID_MOUSE && mouse_report_contains_event(last_mouse_report, sequence->event_press.event.mouse)){
                #if DEBUG_LOG
                ESP_LOGI(pcTaskGetName(NULL), "posthook(): Starting mouse press macro: %s", sequence->timer_args.name);
                #endif
                reset_sequence(sequence);
                start_sequence(sequence);
            }
        }
    }
}

void macro_sequence_callback(void* arg) {
    // Get the key sequence from arguments
    key_modification_sequence_t* key_seq = (key_modification_sequence_t*) arg;
    hid_transmit_t macro_event = key_seq->list[key_seq->pos].event;
    // Copy last humain HID report...
    hid_transmit_t copy_report;
    if (macro_event.header == HEADER_HID_KEYBOARD){
        copy_report.header = HEADER_HID_KEYBOARD;
        copy_report.event.keyboard = last_keyboard_report[0];
    } else if (macro_event.header == HEADER_HID_MOUSE) {
        copy_report.header = HEADER_HID_MOUSE;
        copy_report.event.mouse = last_mouse_report;
        copy_report.event.mouse.x = 0; copy_report.event.mouse.y = 0; copy_report.event.mouse.wheel = 0;
    } else {
        #if DEBUG_LOG
        ESP_LOGI(pcTaskGetName(NULL), "macro_sequence(): Invalid macro sequence n°%d (header: %d): %s", key_seq->pos, macro_event.header, key_seq->timer_args.name);
        #endif
        return;
    }

    // ... and add the custom key to the sequence previous key.
    key_seq->previous_key = macro_event;
    // Previous key of all sequence are added to copy report to send: all sequences can run in parallel
    for (int i = 0; i < MAX_KEY_MODIFICATION_SEQUENCE; i++){
        key_modification_sequence_t* sequence = &group_sequence.list[i];
        // Ignore empty sequence
        if (sequence->size == 0) continue;
        // Skip sequence with not same HID type
        if (sequence->previous_key.header != macro_event.header) continue;
        // Add the keycode to report
        add_event_to_report(&copy_report, sequence->previous_key);
    }
    // In case of mouse, add mouvement to report
    if (macro_event.header == HEADER_HID_MOUSE) set_mouse_movement_to_report(&copy_report.event.mouse, macro_event.event.mouse);

    // Send the modified report to the HID task for USB transmission
    #if DEBUG_LOG
    ESP_LOGI(pcTaskGetName(NULL), "macro_sequence(): Send report from: %s", key_seq->timer_args.name);
    #endif
    hid_add_report(copy_report);

    // Schedule next sequence key with esp_timer
    // Increase the sequence position
    key_seq->pos++;
    // if end of sequence, reset
    if (key_seq->pos >= key_seq->size){
        reset_sequence(key_seq);
        if (key_seq->loop){
            // Ignore end of sequence, try to continue
        } else {
            return;
        }
    }
    // ... else restart timer with next duration
    // (but in a loop the press key must still be pressed)
    if (key_seq->loop && (
         (key_seq->event_press.header == HEADER_HID_KEYBOARD && !keyboard_report_contains_event(last_keyboard_report[0], key_seq->event_press.event.keyboard)) ||
         (key_seq->event_press.header == HEADER_HID_MOUSE && !mouse_report_contains_event(last_mouse_report, key_seq->event_press.event.mouse))
        )
       ){
        reset_sequence(key_seq);
        return;
    }  
    start_sequence(key_seq);
    return;
}
