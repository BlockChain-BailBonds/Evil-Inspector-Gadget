import pathlib
import re
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
class ScreenSafetyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.main=(ROOT/'src/main.cpp').read_text()
        cls.driver=(ROOT/'src/display_st7796.h').read_text()
        cls.pio=(ROOT/'platformio.ini').read_text()
    def test_original_scanner_and_commands_remain(self):
        for token in ['void scanWifi()', 'void scanBle()', 'void initCamera()', 'void writeEvidence()', 'void handle(String command)', 'SCAN WIFI', 'SCAN BLE', 'LOG DUMP']:
            self.assertIn(token,self.main)
    def test_no_auto_display_init(self):
        segment=self.main.split('void setup() {',1)[1].split('void loop() {',1)[0]
        self.assertNotIn('mermaid_lcd::initialize()',segment)
    def test_build_script_accepts_lcd_test_target(self):
        self.assertIn('s3_uart_lcd_test', (ROOT/'tools/build.sh').read_text())
    def test_lcd_flash_application_only_and_gated(self):
        script=(ROOT/'tools/flash_lcd_app_after_backup.sh').read_text()
        for snippet in ['verified_backup', "manifest.get('sha256')", 'FLASH-LCD-S3', '0x10000:.pio/build/s3_uart_lcd_test/firmware.bin', 's3_chipid_probe.py']:
            self.assertIn(snippet,script)
        self.assertNotIn('0x0:.pio/build',script)
        self.assertNotIn('0x8000:.pio/build',script)
    def test_disabled_camera_lcd_test_profile(self):
        env=self.pio.split('[env:s3_uart_lcd_test]',1)[1]
        for flag in ('-DCAMERA_PROFILE=0','-DM3R_LCD_ENABLED=1', '-DARDUINO_USB_CDC_ON_BOOT=0'):
            self.assertIn(flag,env)
    def test_no_unneeded_dep(self):
        self.assertNotIn('lib_deps',self.pio)
        self.assertIn('#include <SPI.h>',self.driver)
    def test_newer_gpio_map_consistent(self):
        env=self.pio.split('[env:s3_uart_lcd_test]',1)[1]
        for name,pin in [('CS',47),('RST',14),('DC',21),('MOSI',41),('SCK',42)]:
            self.assertIn(f'-DM3R_LCD_PIN_{name}={pin}',env)
    def test_full_color_fill_and_reset(self):
        for code in ('command(0x01)', 'command(0x11)', 'command(0x29)', 'SPI.transfer(0x2C)', 'SPI.writeBytes(chunk, count)'):
            self.assertIn(code,self.driver)
    def test_macro_collision_resolved(self):
        self.assertNotIn('static const char HEX[]',self.main)
        self.assertIn('static const char HEX_DIGITS[]',self.main)
    def test_no_gpio_reuse_in_known_safety_set(self):
        self.assertIn('LCD_UNSAFE_PIN',self.driver)
        self.assertIn('CAMERA_PROFILE != 0',self.driver)

if __name__=='__main__': unittest.main()
