#include "dragon_core.h"

static const DragonRadioInfo radios[DRAGON_RADIO_COUNT] = {
    {"Sub-GHz", "subghz"},
    {"NFC 13.56", "nfc"},
    {"RFID 125k", "lfrfid"},
    {"Infrared", "infrared"},
    {"iButton", "ibutton"},
    {"BLE", "bt_settings"},
};

const DragonRadioInfo* dragon_radio_get(DragonRadio radio) {
    if((unsigned)radio >= DRAGON_RADIO_COUNT) return NULL;
    return &radios[radio];
}

bool dragon_is_monitor_action(DragonRadio radio, DragonAction action) {
    return radio == DragonRadioBle && action == DragonActionRead;
}

const char* dragon_native_target(DragonRadio radio, DragonAction action) {
    if((unsigned)radio >= DRAGON_RADIO_COUNT) return NULL;
    if(action != DragonActionRead && action != DragonActionEmulate) return NULL;
    if(dragon_is_monitor_action(radio, action)) return NULL;
    return radios[radio].native_app_id;
}

void dragon_filter_line(char* dest, size_t capacity, const char* src) {
    if(!dest || capacity == 0) return;
    size_t i = 0;
    if(src) {
        while(src[i] && i + 1 < capacity) {
            const unsigned char c = (unsigned char)src[i];
            dest[i] = (c >= 32 && c <= 126) ? (char)c : ' ';
            i++;
        }
    }
    dest[i] = '\0';
}
