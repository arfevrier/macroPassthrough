// Import global project config
#include "config.h"

hid_keyboard_report_t last_report[2];
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

void macro_prehook_transmission(hid_keyboard_report_t* report){
    
    // TODO -- User modification
    if (keycode_contains_key(*report, HID_KEY_A) && keycode_contains_key(*report, HID_KEY_D)){
        if (keycode_contains_key(last_report[0], HID_KEY_A)){
            remove_keycode(report, HID_KEY_A);
        } else if (keycode_contains_key(last_report[0], HID_KEY_D)){
            remove_keycode(report, HID_KEY_D);
        }
    }
    // -- END

    // Example: Remove a KEY
    // remove_keycode(report, HID_KEY_C);
}

void macro_posthook_transmission(hid_keyboard_report_t* report){
    // Update last report transmission, like this all macro are added to real keys press by user
    last_report[1] = last_report[0];
    last_report[0] = *report;

    // Manage sequence start:
    for (int i = 0; i < MAX_KEY_MODIFICATION_SEQUENCE; i++){
        key_modification_sequence_t* sequence = &group_sequence.list[i];
        // Ignore empty sequence
        if (sequence->size == 0) continue;
        // If a press key is defined for sequence
        if (sequence->detect_key != 0 && keycode_contains_key(last_report[0], sequence->detect_key)){
            esp_timer_start_once(sequence->timer, sequence->list[sequence->pos].duration);
        }
        // If a unpress key is defined for sequence
        if (sequence->detect_unkey != 0 && keycode_contains_key(last_report[1], sequence->detect_unkey) && !keycode_contains_key(last_report[0], sequence->detect_unkey)){
            esp_timer_start_once(sequence->timer, sequence->list[sequence->pos].duration);
        }
    }
}

void macro_sequence_callback(void* arg) {
    // Get the key sequence from arguments
    key_modification_sequence_t* key_seq = (key_modification_sequence_t*) arg;
    // Copy last humain HID report...
    hid_keyboard_report_t copy_report = last_report[0];

    // ... and add the custom key to the last report.
    // (like this we keep humain keyboard working during a sequence)
    add_keycode(&copy_report, key_seq->list[key_seq->pos].key);

    // Inform of the key sequence event
    key_seq->previous_key = key_seq->list[key_seq->pos].key;
    // Add all current keys sequences: like this all sequences can run in parallel
    for (int i = 0; i < MAX_KEY_MODIFICATION_SEQUENCE; i++){
        key_modification_sequence_t* sequence = &group_sequence.list[i];
        // Ignore empty sequence
        if (sequence->size == 0) continue;
        // Skip sequence containing current key (already added)
        if (sequence->previous_key == key_seq->list[key_seq->pos].key) continue;
        // Add the keycode to report
        add_keycode(&copy_report, sequence->previous_key);
    }

    // Send the modified report to the HID task for USB transmission
    static hid_transmit_t transmit_report;
    transmit_report.header = HEADER_HID_KEYBOARD_TRANSMISSION;
    transmit_report.event.keyboard = copy_report;
    hid_add_report(transmit_report);

    // Schedule next sequence key with esp_timer
    // Increase the sequence position
    key_seq->pos++;
    if (key_seq->pos < key_seq->size){
        // if not the end of sequence, restart timer with next duration
        esp_timer_start_once(key_seq->timer, key_seq->list[key_seq->pos].duration);
    } else {
        // if end of sequence, reset
        key_seq->pos = 0;
        //  but for loop sequence, continue if key still pressed
        if (key_seq->loop && keycode_contains_key(last_report[0], key_seq->detect_key)){
            esp_timer_start_once(key_seq->timer, key_seq->list[key_seq->pos].duration);
        }
    }
}
