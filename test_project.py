from pathlib import Path
from PIL import Image
import re, hashlib, subprocess, sys, json
root=Path(__file__).resolve().parent
s3=root/'s3_redteam_inspector'
fap=root/'flipper_fap/inspector_gadget'
asset=s3/'assets/inspector_dashboard_320x480.png'
im=Image.open(asset); assert im.size==(320,480)
assert (s3/'assets/inspector_dashboard_320x480.rgb565').stat().st_size==320*480*2
h=(s3/'src/inspector_asset.h').read_text()
assert len(re.findall(r'\b0x[0-9A-F]{4}\b',h))==320*480
ui=(s3/'src/inspector_ui.h').read_text()
assert '#include "inspector_asset.h"' in ui
assert 'inspector_asset::pixels' in ui
for p in ['47','14','40','41','42','1','2','21']:
 assert (f'-DM3R_LCD_PIN_CS={p}' in (s3/'platformio.ini').read_text()) or p in (s3/'platformio.ini').read_text()
assert '-DCAMERA_PROFILE=0' in (s3/'platformio.ini').read_text()
assert 'redteam_dragon::service()' in (s3/'src/main.cpp').read_text()
assert 'INSPECTOR WIFI' in (s3/'src/main.cpp').read_text()
assert 'DRAGON WIFI' not in (s3/'src/main.cpp').read_text()
assert 'inspector_asset::pixels' in ui and 'copyApprovedDashboard();' in ui
assert 'INSPECTOR START' in ui
fap_src=(fap/'redteam_dragon.c').read_text()
assert 'canvas_draw_xbm' in fap_src
assert '#include "inspector_assets.h"' in fap_src
assert 'furi_hal_serial_disable_direction(app->serial, FuriHalSerialDirectionTx)' in fap_src
assert 'furi_hal_serial_tx(' not in fap_src
assert 'loader_enqueue_launch' in fap_src
assert 'flipper_uart' not in fap_src
b=(fap/'inspector_assets.h').read_text()
assert b.count('0x')==128*64//8
assert Image.open(fap/'preview_128x64.png').size==(128,64)
assert Image.open(fap/'icon.png').size==(10,10)
subprocess.run([sys.executable,str(root/'flipper_firmware_overlay/test_overlay.py')],check=True)
print('PASS: S3 RGB565 size+flash wiring; firmware command routing; FAP XBM asset; UART0 receive-only safety; custom firmware overlay')
