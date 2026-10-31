#include "inspector_link.h"
#include <stddef.h>
#include <string.h>
#include <limits.h>

static bool prefix(const char** cursor, const char* text) {
    size_t n = strlen(text);
    if(strncmp(*cursor, text, n)) return false;
    *cursor += n;
    return true;
}
static bool number(const char** cursor, uint32_t limit, uint32_t* out) {
    uint32_t result = 0;
    const char* p = *cursor;
    if(*p < '0' || *p > '9') return false;
    do {
        const uint32_t digit = (uint32_t)(*p - '0');
        if(result > (limit - digit) / 10U) return false;
        result = result * 10U + digit;
        ++p;
    } while(*p >= '0' && *p <= '9');
    *cursor = p;
    *out = result;
    return true;
}
bool inspector_link_parse(const char* line, InspectorLinkFrame* frame) {
    if(!line || !frame || strlen(line) > 100U) return false;
    const char* p = line;
    uint32_t up, wifi, ble;
    if(!prefix(&p, "EIG918|1|STAT|up=")) return false;
    if(!number(&p, UINT32_MAX, &up)) return false;
    if(!prefix(&p, "|wifi=")) return false;
    if(!number(&p, UINT16_MAX, &wifi)) return false;
    if(!prefix(&p, "|ble=")) return false;
    if(!number(&p, UINT16_MAX, &ble)) return false;
    if(*p != '\0') return false;
    frame->valid = true;
    frame->uptime_s = up;
    frame->wifi_saved = (uint16_t)wifi;
    frame->ble_saved = (uint16_t)ble;
    return true;
}
