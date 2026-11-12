from pathlib import Path
import unittest

R=Path(__file__).resolve().parents[1]
class TouchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.main=(R/'src/main.cpp').read_text()
        cls.touch=(R/'src/touch_ft6336.h').read_text()
        cls.display=(R/'src/display_st7796.h').read_text()
        cls.pio=(R/'platformio.ini').read_text()
        cls.flash=(R/'tools/flash_touch_app_after_backup.sh').read_text()
    def test_scanner_preserved(self):
        for s in ('void scanWifi()', 'void scanBle()', 'void initCamera()', 'void writeEvidence()', 'SCAN WIFI', 'SCAN BLE', 'LOG DUMP'):
            self.assertIn(s,self.main)
    def test_touch_commands_reachable(self):
        self.assertIn('cmd == "TOUCH DIAG"', self.touch)
        for s in ('TOUCH STATUS','TOUCH PROBE','TOUCH INIT','TOUCH READ','TOUCH TEST','TOUCH STOP','TOUCH ROTATE'):
            self.assertIn(s,self.touch)
        self.assertIn('mermaid_touch::handle(command, Serial)',self.main)
        self.assertIn('mermaid_touch::service(Serial)',self.main)
    def test_no_autostart_i2c_or_lcd(self):
        setup=self.main.split('void setup() {',1)[1].split('void loop() {',1)[0]
        for name in ('mermaid_touch::init(', 'mermaid_touch::startBus(', 'mermaid_lcd::initialize()'):
            self.assertNotIn(name,setup)
    def test_camera_disabled_and_pins_disjoint(self):
        env=self.pio.split('[env:s3_uart_touch_test]',1)[1]
        self.assertIn('-DCAMERA_PROFILE=0',env)
        lcd=[47,14,21,41,42]
        touch=[1,2,40]
        self.assertFalse(set(lcd).intersection(touch))
        for name,value in [('SDA',1),('SCL',2),('RST',40)]:
            self.assertIn(f'-DM3R_TOUCH_PIN_{name}={value}',env)
        self.assertIn('Touch GPIOs must not overlap',self.touch)
    def test_focaltech_7bit_address_and_regs(self):
        for s in ('0x38','readRegisters(0x02','readRegisters(0xA8','readRegisters(0xA3', 'point.rawX >= MAX_X || point.rawY >= MAX_Y'):
            self.assertIn(s,self.touch)
    def test_i2c_ack_and_partial_read_fail_closed(self):
        for s in ('Wire.endTransmission() == 0','Wire.endTransmission(false) != 0','received != count', 'TOUCH INIT FAIL', 'TOUCH TEST WARN'):
            self.assertIn(s,self.touch)
    def test_polling_no_irq_needed(self):
        self.assertIn('irq=UNWIRED',self.touch)
        self.assertNotIn('attachInterrupt(',self.touch)
    def test_touch_demo_nonblocking(self):
        self.assertIn('TEST_MS = 30000',self.touch)
        self.assertIn('POLL_MS = 35',self.touch)
        self.assertIn('now-testStarted>=TEST_MS',self.touch)
    def test_writes_app_only_and_backup_gate(self):
        for s in ('verified_backup',"manifest.get('sha256')", 'FLASH-TOUCH-S3','0x10000:.pio/build/s3_uart_touch_test/firmware.bin','s3_chipid_probe.py'):
            self.assertIn(s,self.flash)
        for s in ('0x0:.pio/build','0x8000:.pio/build'):
            self.assertNotIn(s,self.flash)
    def test_build_target_available(self):
        self.assertIn('s3_uart_touch_test', (R/'tools/build.sh').read_text())
    def test_color_bars_untouched(self):
        for s in ('inline void bars()', '0xF800', '0x07E0', '0x001F'):
            self.assertIn(s,self.display)
    def test_legacy_hex_collision_not_reintroduced(self):
        self.assertNotIn('static const char HEX[]',self.main)
        self.assertIn('HEX_DIGITS',self.main)
    def test_native_320x480_to_480x320_transform(self):
        self.assertIn('point.screenX = y;',self.touch)
        self.assertIn('point.screenY = MAX_X - 1 - x;',self.touch)
    def test_no_touch_camera_autostart(self):
        self.assertNotIn('Wire.begin(',self.main)
        self.assertIn('Wire.begin(M3R_TOUCH_PIN_SDA',self.touch)

if __name__=='__main__':unittest.main()
