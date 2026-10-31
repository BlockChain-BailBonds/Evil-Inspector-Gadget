#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    bool valid;
    uint32_t uptime_s;
    uint16_t wifi_saved;
    uint16_t ble_saved;
} InspectorLinkFrame;
// Validates a complete text line: EIG918|1|STAT|up=<u32>|wifi=<u16>|ble=<u16>
// Does not accept suffix fields, negative numbers, overflow or unbounded payloads.
bool inspector_link_parse(const char* line, InspectorLinkFrame* frame);
