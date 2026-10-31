import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "backup_s3_flash.py"
spec = importlib.util.spec_from_file_location("backup_s3_flash", SCRIPT)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

class FakeROM:
    @staticmethod
    def _slip_decode(data):
        assert data[0] == data[-1] == 0xc0
        return data[1:-1].replace(b'\xdb\xdc', b'\xc0').replace(b'\xdb\xdd', b'\xdb')

class FakeLoader:
    def __init__(self, payload, tamper=False):
        self.payload = payload
        self.read_at = 0
        self.ack = []
        self.tamper = tamper
    def _command(self, op, data, **kw):
        assert op == 0xd2
        offset, length, sector, maxflight = struct.unpack('<IIII',data)
        assert offset == 0x10000
        assert length == len(self.payload)
        assert sector == 4096 and maxflight == 1
        return (0,b'\x00\x00')
    def _slip_read_packet(self, timeout):
        if self.read_at < len(self.payload):
            chunk = self.payload[self.read_at:self.read_at+4096]
            self.read_at += len(chunk)
            return chunk
        digest = hashlib.md5(self.payload).digest()
        return bytes([digest[0]^1])+digest[1:] if self.tamper else digest
    def _slip_encode(self,p):
        out = bytearray(b'\xc0')
        for x in p:
            if x == 0xC0:
                out.extend(b'\xdb\xdc')
            elif x == 0xDB:
                out.extend(b'\xdb\xdd')
            else:
                out.append(x)
        out.append(0xC0)
        return bytes(out)
    def _write(self,p,timeout):
        assert p[:1] == b'\xc0' and p[-1:] == b'\xc0', 'ACK must be SLIP-framed'
        body = p[1:-1].replace(b'\xdb\xdc', b'\xc0').replace(b'\xdb\xdd', b'\xdb')
        assert len(body) == 4, 'ACK payload must be exactly four bytes'
        self.ack.append(struct.unpack('<I',body)[0])
        return len(p)

class BackupTests(unittest.TestCase):
    def test_slip_reader_retains_next_frame(self):
        frames = [b'garbage\xc0Hi\xc0\xc0X\xc0']
        b = mod.BufferedSlipReader(lambda n, t: frames.pop(0) if frames else b'', type('ROM',(),{'RomLoader':FakeROM}))
        self.assertEqual(b.read_packet(.1),b'Hi')
        self.assertEqual(b.read_packet(.1),b'X')
    def test_chunk_digest_and_ack(self):
        payload = bytes(range(256))*32
        rom = FakeLoader(payload)
        with tempfile.TemporaryFile() as f:
            mod.read_verified_chunk(rom,0x10000,len(payload),f)
            f.seek(0)
            self.assertEqual(f.read(), payload)
        self.assertEqual(rom.ack,[4096,8192])
    def test_cumulative_ack_with_c0_byte_is_escaped(self):
        payload = bytes(range(256)) * 256   # 64 KiB; 0xc000 ACK needs SLIP escape
        rom = FakeLoader(payload)
        with tempfile.TemporaryFile() as f:
            mod.read_verified_chunk(rom, 0x10000, len(payload), f)
            f.seek(0)
            self.assertEqual(f.read(), payload)
        self.assertIn(0xc000, rom.ack)
        self.assertEqual(rom.ack[-1], len(payload))

    def test_ack_short_usb_write_aborts_readback(self):
        rom = FakeLoader(bytes(range(256))*16)
        rom._write = lambda payload, timeout: len(payload) - 1
        with tempfile.TemporaryFile() as f:
            with self.assertRaisesRegex(mod.BackupError, 'Short SLIP-framed ACK'):
                mod.read_verified_chunk(rom,0x10000,4096,f)

    def test_keep_known_good_rom_reader_until_ram_stub_ready(self):
        source = SCRIPT.read_text()
        sync = source.index('nf.enter_bootloader_and_sync(')
        security = source.index('GET_SECURITY_INFO')
        stub = source.index('loader.upload_stub(stub)')
        switch = source.index('loader._slip_read_packet = buffered.read_packet')
        self.assertLess(sync, security)
        self.assertLess(security, stub)
        self.assertLess(stub, switch)

    def test_s3_identity_check_uses_rom_security_info_not_legacy_magic(self):
        source = SCRIPT.read_text()
        self.assertIn('GET_SECURITY_INFO', source)
        self.assertIn('chip_id != 9', source)
        self.assertNotIn('nf.resolve_chip(loader', source)

    def test_termux_log_precreated_before_launch(self):
        source = SCRIPT.read_text()
        start = source.index('def termux_bootstrap(')
        section = source[start:source.index('def main()', start)]
        self.assertLess(section.index('with open(log_file, "x"'), section.index('eb.launch_with_fd('))

    def test_flash_script_checks_s3_security_identity(self):
        script = (SCRIPT.parent / 'flash_after_backup.sh').read_text()
        self.assertIn('s3_chipid_probe.py', script)
        self.assertNotIn('nrflash probe', script)
        self.assertIn('nrflash write --chip esp32s3 --verify', script)

    def test_chunk_bad_digest_refused(self):
        rom=FakeLoader(b'\x02'*4096,tamper=True)
        with tempfile.TemporaryFile() as f:
            with self.assertRaises(mod.BackupError):
                mod.read_verified_chunk(rom,0x10000,4096,f)

if __name__ == '__main__':
    unittest.main()
