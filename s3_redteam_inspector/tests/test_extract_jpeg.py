import base64
import sys
from pathlib import Path
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
from extract_jpeg import recover

class TestExtract(unittest.TestCase):
    def setUp(self):
        self.image = b"\xff\xd8\xff\xe0fake-jpeg-for-parser\xff\xd9"
        self.crc = zlib.crc32(self.image)
        self.log = (f"CAM_BEGIN bytes={len(self.image)} crc32={self.crc:08X} format=jpeg\n"
                    + "CAM_DATA " + base64.b64encode(self.image).decode() + "\n"
                    + f"CAM_END bytes={len(self.image)} crc32={self.crc:08X}\n")

    def test_verified_recovery(self):
        self.assertEqual(recover(self.log), self.image)

    def test_damaged_data_rejected(self):
        changed = self.log.replace("CAM_DATA ", "CAM_DATA AA")
        with self.assertRaises(ValueError):
            recover(changed)

    def test_truncated_data_rejected(self):
        with self.assertRaises(ValueError):
            recover(self.log.replace("CAM_END", "BROKEN_END"))

    def test_empty_log_rejected(self):
        with self.assertRaises(ValueError):
            recover("M3rMa1d S3-CAM\nSTATUS\n")

    def test_last_complete_frame_selected(self):
        self.assertEqual(recover(self.log + self.log), self.image)

    def test_prefix_noise(self):
        noisy = "[received] " + self.log.replace("\n", "\n[received] ")
        self.assertEqual(recover(noisy), self.image)

if __name__ == "__main__":
    unittest.main()
