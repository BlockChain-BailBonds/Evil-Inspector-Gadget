import tempfile
import unittest
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
from pack_rom import package, SIZE

class TestRom(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.binary = self.root / "bin"
        self.binary.mkdir()
        (self.binary / "bootloader.bin").write_bytes(b"B" * 1024)
        (self.binary / "partitions.bin").write_bytes(b"P" * 512)
        (self.binary / "firmware.bin").write_bytes(b"A" * 8192)

    def test_exact_offsets(self):
        out = self.root / "test.bin"
        image = package(self.binary, out)
        self.assertEqual(len(image), SIZE)
        self.assertEqual(image[0:1024], b"B" * 1024)
        self.assertEqual(image[0x8000:0x8200], b"P" * 512)
        self.assertEqual(image[0x10000:0x12000], b"A" * 8192)
        self.assertEqual(image[0xF000:0x10000], b"\xFF" * 4096)
        self.assertEqual(out.stat().st_size, SIZE)

    def test_no_overwrite(self):
        out = self.root / "already.bin"
        out.write_bytes(b"KEEP")
        with self.assertRaises(FileExistsError):
            package(self.binary, out)
        self.assertEqual(out.read_bytes(), b"KEEP")

    def test_oversized_app_rejected(self):
        (self.binary / "firmware.bin").write_bytes(b"A" * (0x400000 + 1))
        with self.assertRaises(ValueError):
            package(self.binary, self.root / "oversized.bin")

if __name__ == "__main__":
    unittest.main()
