#include "inspector_link.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
int main(void) {
    InspectorLinkFrame f = {0};
    assert(inspector_link_parse("EIG918|1|STAT|up=2|wifi=33|ble=7", &f));
    assert(f.valid && f.uptime_s == 2 && f.wifi_saved == 33 && f.ble_saved == 7);
    const char* bad[] = {
        "EIG918|1|STAT|up=1|wifi=1|ble=1|evil=1",
        "EIG918|2|STAT|up=1|wifi=1|ble=1",
        "EIG918|1|STAT|up=-1|wifi=1|ble=1",
        "EIG918|1|STAT|up=4294967296|wifi=1|ble=1",
        "EIG918|1|STAT|up=1|wifi=65536|ble=1",
        "EIG918|1|STAT|up=1|wifi=0|ble=999999",
        "EIG918|1|STAT|up=1|wifi=1|ble=",
        "EIG918|1|STAT|up=1|wifi=1|ble=1\\r",
        "EXTERNAL|1|STAT|up=1|wifi=1|ble=1",
    };
    for(size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); ++i) {
        assert(!inspector_link_parse(bad[i], &f));
    }
    assert(!inspector_link_parse(NULL,&f));
    assert(!inspector_link_parse("EIG918|1|STAT|up=1|wifi=1|ble=1",NULL));
    assert(inspector_link_parse("EIG918|1|STAT|up=4294967295|wifi=65535|ble=65535",&f));
    puts("PASS inspector UART text protocol parser");
    return 0;
}
