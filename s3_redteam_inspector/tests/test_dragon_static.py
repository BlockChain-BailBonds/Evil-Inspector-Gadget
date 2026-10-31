from pathlib import Path
import re, struct, unittest, hashlib

ROOT=Path(__file__).resolve().parents[1]

class DragonContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.main=(ROOT/'src/main.cpp').read_text()
        cls.ui=(ROOT/'src/dragon_ui.h').read_text()
        cls.ini=(ROOT/'platformio.ini').read_text()
        cls.flash=(ROOT/'tools/flash_dragon_app_after_backup.sh').read_text()
    def test_asset_exact_size(self):
        self.assertEqual((ROOT/'assets/dragon_dashboard_320x480.rgb565').stat().st_size,320*480*2)
    def test_image_asset_in_flash(self):
        self.assertIn('PROGMEM', (ROOT/'src/dragon_asset.h').read_text())
        self.assertIn('memcpy_P(frame',self.ui)
    def test_image_original_preserved(self):
        self.assertTrue((ROOT/'assets/approved_redteam_dragon_mockup.png').is_file())
    def test_flash_application_only(self):
        self.assertIn('0x10000:', self.flash)
        self.assertNotIn('0x0:', self.flash)
        self.assertIn('FLASH-DRAGON-S3', self.flash)
        self.assertIn('verified_backup', self.flash)
    def test_camera_disabled_and_uart(self):
        self.assertRegex(self.ini,r'(?s)\[env:s3_uart_dragon\].*?-DCAMERA_PROFILE=0')
        self.assertRegex(self.ini,r'(?s)\[env:s3_uart_dragon\].*?-DARDUINO_USB_CDC_ON_BOOT=0')
    def test_gpio_unchanged_from_touch_baseline(self):
        section=self.ini.split('[env:s3_uart_dragon]')[1]
        values=['47','14','40','41','42','1','2','21']
        for name,value in zip(['LCD_PIN_CS','LCD_PIN_RST','LCD_PIN_DC','LCD_PIN_MOSI','LCD_PIN_SCK','TOUCH_PIN_SDA','TOUCH_PIN_SCL','TOUCH_PIN_RST'],values):
            self.assertIn('-DM3R_'+name+'='+value,section)
    def test_guard_mode(self):
        self.assertIn('#error "RedTeam Dragon requires working LCD + touch and camera disabled"',self.ui)
    def test_no_unsafe_wireless(self):
        self.assertNotIn('WiFi.begin(',self.ui)
        self.assertNotIn('setActiveScan(true)',self.ui)
    def test_touch_event_debounce(self):
        self.assertIn('touchWasDown=true',self.ui)
        self.assertIn('now-lastTap<230',self.ui)
    def test_existing_usb_commands_retained(self):
        for cmd in ['SCAN WIFI','SCAN BLE','TOUCH TEST','LCD TEST','CAM STATUS','EXPORT']:
            self.assertIn(cmd,self.main)
    def test_json_hex_patch_preserved(self):
        self.assertIn('HEX_DIGITS',self.main)
        self.assertNotIn('const char HEX[]',self.main)
    def test_source_bridge_definitions_are_global(self):
        close_anon=self.main.index('\n}\n\n#if defined(M3R_DRAGON_ENABLED) && M3R_DRAGON_ENABLED\n// These bridge')
        bridge=self.main.index('namespace redteam_dragon {\nvoid refreshWifi(){')
        self.assertGreater(bridge,close_anon)
    def test_verified_rollback_script(self):
        rollback=(ROOT/'tools/rollback_to_v107_after_backup.sh').read_text()
        self.assertIn('RESTORE-S3-V107',rollback)
        self.assertIn('71d8864559ac6b710b9e34c8c616e2a76bf45e0b14b5ab307c7235c9c1134bf5',rollback)
        self.assertIn('verified_backup',rollback)
        self.assertIn('0x10000:',rollback)
    def test_4mb_app_partition(self):
        self.assertIn('0x10000 + app.stat().st_size > 0x410000',self.flash)
    def test_layout_portrait(self):
        self.assertIn('constexpr uint16_t W=320, H=480',self.ui)
        self.assertIn('frame[size_t(479-x)*W+y]',self.ui)
    def test_page_handlers(self):
        for page in ['drawHome','drawWifi','drawBle','drawCamera','drawLogs','drawSettings','drawStatus']:
            self.assertIn(page+'()',self.ui)

if __name__=='__main__':unittest.main()
