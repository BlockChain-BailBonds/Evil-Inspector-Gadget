#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DRAGON_RADIO_COUNT 6u
#define DRAGON_LINE_CAP 48u

typedef enum {
    DragonRadioSubGhz = 0,
    DragonRadioNfc,
    DragonRadioLfRfid,
    DragonRadioInfrared,
    DragonRadioIbutton,
    DragonRadioBle,
} DragonRadio;

typedef enum {
    DragonActionRead = 0,
    DragonActionEmulate = 1,
} DragonAction;

typedef struct {
    const char* label;
    const char* native_app_id;
} DragonRadioInfo;

const DragonRadioInfo* dragon_radio_get(DragonRadio radio);
const char* dragon_native_target(DragonRadio radio, DragonAction action);
bool dragon_is_monitor_action(DragonRadio radio, DragonAction action);
void dragon_filter_line(char* dest, size_t capacity, const char* src);
