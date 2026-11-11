from pathlib import Path
import re
root=Path(__file__).resolve().parent
s3=(root/'s3_redteam_inspector/src/main.cpp').read_text()
fap=(root/'flipper_fap/inspector_gadget/redteam_dragon.c').read_text()
ui=(root/'s3_redteam_inspector/src/inspector_ui.h').read_text()
for token in ['EIG918|1|HELLO','EIG918|1|STAT|up=%lu|wifi=%u|ble=%u', 'LINK STATUS', 'CAMERA_PROFILE=0']:
    assert token in s3 or token in (root/'s3_redteam_inspector/platformio.ini').read_text(),token
assert 'inspector_asset::pixels' in ui
assert 'inspector_link_parse(app->building, &parsed)' in fap
assert 'furi_hal_serial_enable_direction(app->serial, FuriHalSerialDirectionTx)' in fap
assert 'furi_hal_serial_tx(' in fap
assert 'flipperSerial.begin(FLIPPER_BAUD, SERIAL_8N1, FLIPPER_RX_PIN, FLIPPER_TX_PIN)' in s3
assert 'serviceFlipperLink();' in s3
assert 'FLIPPER_RX_PIN = 3;' in s3 and 'FLIPPER_TX_PIN = 7;' in s3
assert '-DM3R_TOUCH_PIN_RST=-1' in (root/'s3_redteam_inspector/platformio.ini').read_text()
assert 'furi_hal_serial_async_rx_available(handle)' in fap
flash=(root/'s3_redteam_inspector/tools/flash_inspector_after_backup.sh').read_text()
assert '0x10000:.pio/build/s3_uart_inspector/firmware.bin' in flash
assert '0x10000:.pio/build/s3_uart_dragon/firmware.bin' not in flash
assert 'FLASH-INSPECTOR-S3' in flash
assert 'loader_enqueue_launch' in fap
assert 'furi_get_tick()' in fap
assert '#include "inspector_assets.h"' in fap
print('PASS bidirectional UART1 TX/RX FAP and isolated S3 Inspector artwork linked')
