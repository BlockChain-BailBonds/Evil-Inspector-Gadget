from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent
S3 = (ROOT / "src/main.cpp").read_text()
TOUCH = (ROOT / "src/touch_ft6336.h").read_text()
CFG = (ROOT / "platformio.ini").read_text()
FAP = (REPO / "flipper_fap/inspector_gadget/redteam_dragon.c").read_text()


class FlipperDuplexContract(unittest.TestCase):
    def test_uart_separate_from_usb_ch340(self):
        self.assertIn("HardwareSerial flipperSerial(1)", S3)
        self.assertIn("FLIPPER_RX_PIN = 3", S3)
        self.assertIn("FLIPPER_TX_PIN = 7", S3)
        self.assertIn("flipperSerial.begin(FLIPPER_BAUD, SERIAL_8N1, FLIPPER_RX_PIN, FLIPPER_TX_PIN)", S3)
        self.assertIn("Serial.begin(115200)", S3)
        self.assertNotIn("flipperSerial.begin(FLIPPER_BAUD, SERIAL_8N1, 44, 43)", S3)

    def test_display_touch_and_uart_gpio_do_not_overlap(self):
        display = {47, 14, 21, 41, 42}
        touch = {38, 39}
        flipper = {3, 7}
        sdmmc = {38, 39, 40}
        camera = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18}
        usb_ch340 = {19, 20, 43, 44}
        self.assertFalse(display & touch)
        self.assertFalse(display & flipper)
        self.assertFalse(touch & flipper)
        self.assertFalse(flipper & usb_ch340)
        inspector = CFG.split("[env:s3_uart_inspector]", 1)[1]
        self.assertIn("-DM3R_TOUCH_PIN_SDA=38", inspector)
        self.assertIn("-DM3R_TOUCH_PIN_SCL=39", inspector)
        self.assertIn("-DM3R_TOUCH_PIN_RST=-1", inspector)
        self.assertIn("-DM3R_LCD_PIN_BL=27", inspector)
        self.assertIn("-DCAMERA_PROFILE=0", inspector)
        self.assertNotIn("-DM3R_TOUCH_PIN_RST=40", inspector)
        self.assertIn("#if M3R_TOUCH_PIN_RST >= 0", TOUCH)

    def test_uart_rx_protocol_bounded(self):
        self.assertIn("char flipperInput[64]", S3)
        self.assertIn("flipperDiscardLine = true", S3)
        self.assertIn("flipperLength + 1 >= sizeof(flipperInput)", S3)
        self.assertIn('EIG918|1|ERR|INVALID_FRAME', S3)
        self.assertIn('EIG918|1|ERR|UNKNOWN_COMMAND', S3)
        self.assertIn('void serviceFlipperLink()', S3)
        self.assertIn('serviceFlipperLink();', S3)
        self.assertIn('flipperSendStatus();', S3)

    def test_finite_flipper_actions_and_bidirectional_direction(self):
        self.assertIn("furi_hal_serial_enable_direction(app->serial, FuriHalSerialDirectionTx)", FAP)
        self.assertIn("furi_hal_serial_tx(app->serial", FAP)
        self.assertIn("furi_hal_serial_tx_wait_complete", FAP)
        for op in ("PING", "STATUS", "HOME", "WIFI", "BLE"):
            self.assertIn(f'dragon_send_command(app, "{op}")', FAP)
            self.assertIn(f'strcmp(operation, "{op}")', S3)
        self.assertNotIn("furi_hal_serial_disable_direction(app->serial, FuriHalSerialDirectionTx)", FAP)


if __name__ == "__main__":
    unittest.main()
