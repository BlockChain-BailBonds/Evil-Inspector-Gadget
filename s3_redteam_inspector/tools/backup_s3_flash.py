#!/usr/bin/env python3
"""Read and verify a complete ESP32-S3 16-MiB SPI NOR flash backup.

Uses the installed nrflash ROM/stub and espbridge USB transport, which support
no-root Samsung Termux via termux-usb. This program NEVER writes or erases flash.

Run: python3 tools/backup_s3_flash.py
Backup: ~/m3rmaid-s3-backups/<timestamp>-original-16m.bin
Requires: pip install nrflash espbridge; Termux:API and USB permission.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import struct
import sys
import time

SIZE = 16 * 1024 * 1024
CHUNK = 64 * 1024
SECTOR = 4096
CMD_STUB_READ_FLASH = 0xD2


class BackupError(RuntimeError):
    pass


class BufferedSlipReader:
    """Keep all bytes after a SLIP frame, including early flash-data frames."""
    def __init__(self, read_fn, rom_loader):
        self.read_fn = read_fn
        self.rom_loader = rom_loader
        self.pending = bytearray()

    def read_packet(self, timeout: float) -> bytes:
        deadline = time.monotonic() + timeout
        while True:
            begin = self.pending.find(b"\xc0")
            if begin >= 0:
                if begin:
                    del self.pending[:begin]
                end = self.pending.find(b"\xc0", 1)
                if end >= 0:
                    frame = bytes(self.pending[:end + 1])
                    del self.pending[:end + 1]
                    body = self.rom_loader.RomLoader._slip_decode(frame)
                    if body:
                        return body
                    continue
            elif self.pending:
                self.pending.clear()  # disregard ROM boot messages before SLIP
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise BackupError("Timed out reading ESP32-S3 SLIP frame; USB may have disconnected")
            received = self.read_fn(8192, min(2000, max(1, int(remaining * 1000))))
            if received:
                self.pending.extend(received)
                if len(self.pending) > SECTOR * 8:
                    raise BackupError("Excessive buffered USB data; refusing an ambiguous backup")


def read_verified_chunk(loader, offset: int, length: int, target) -> None:
    """S3 stub READ_FLASH: 4 args; SLIP data, framed four-byte ACKs, SLIP MD5."""
    if length <= 0 or length % SECTOR or offset % SECTOR:
        raise BackupError("Read chunks must be 4096-byte aligned")
    request = struct.pack("<IIII", offset, length, SECTOR, 1)
    loader._command(CMD_STUB_READ_FLASH, request, timeout=8.0, retries=1)
    received = 0
    digest = hashlib.md5()
    while received < length:
        packet = loader._slip_read_packet(timeout=15.0)
        expected = min(SECTOR, length - received)
        if len(packet) != expected:
            raise BackupError(
                f"Corrupted packet @ 0x{offset + received:06X}: "
                f"expected {expected} bytes, got {len(packet)}"
            )
        target.write(packet)
        digest.update(packet)
        received += len(packet)
        # Espressif's esptool.write() SLIP-frames and escapes each 4-byte
        # cumulative ACK. Raw bytes are NOT valid: the stub waits for a SLIP
        # frame and then stops after its allowed in-flight packet count.
        ack = struct.pack("<I", received)
        framed_ack = loader._slip_encode(ack)
        written = loader._write(framed_ack, 5000)
        if written != len(framed_ack):
            raise BackupError("Short SLIP-framed ACK write during flash backup")
    expected_md5 = loader._slip_read_packet(timeout=15.0)
    if len(expected_md5) != 16 or expected_md5 != digest.digest():
        raise BackupError(
            f"Device returned mismatched MD5 for 0x{offset:06X}-0x{offset+length:06X}"
        )


def backup_to(output: Path) -> None:
    from nrflash import cli as nf, rom_loader, stub_flasher_data
    import usb.core

    if output.exists() or output.with_name(output.name + ".partial").exists():
        raise BackupError("Backup destination or partial already exists; refusing overwrite")
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.parent.stat().st_mode & 0o002:
        print("WARNING: Backup directory is world-writable; keep firmware private", flush=True)

    print("[1/4] Connecting to physical ESP32-S3 (READ ONLY)", flush=True)
    device, ep_in, ep_out, reset = nf.acquire_device()
    raw_read, raw_write = nf.make_read_write(device, ep_in, ep_out)
    buffered = BufferedSlipReader(raw_read, rom_loader)
    loader = rom_loader.RomLoader(raw_read, raw_write, "esp32s3")
    partial = output.with_name(output.name + ".partial")
    success = False
    try:
        print("[1a/4] Resetting to ROM download mode and syncing using nrflash's standard reader", flush=True)
        if not nf.enter_bootloader_and_sync(device, loader, reset):
            raise BackupError("Could not enter ESP ROM bootloader on this USB port")
        print("[1b/4] ROM sync successful; querying S3 chip ID", flush=True)
        # nrflash's legacy ROM-magic register can misidentify an S3 behind a
        # CH340 adapter. The S3 ROM's security-info command provides chip ID 9.
        # We fail closed if it cannot be read, rather than trusting the magic.
        try:
            _value, security_body = loader._command(
                0x14, b"", timeout=3.0, retries=2
            )
        except Exception as exc:
            raise BackupError(
                "Cannot verify S3 chip ID via GET_SECURITY_INFO; "
                "stop rather than trusting the legacy nrflash magic"
            ) from exc
        if len(security_body) < 20:
            raise BackupError(
                f"Short GET_SECURITY_INFO response ({len(security_body)} bytes)"
            )
        chip_id = struct.unpack_from("<I", security_body, 12)[0]
        if chip_id != 9:
            raise BackupError(f"Wrong chip ID {chip_id}; expected S3 ID 9")
        chip = "esp32s3"
        print("CONFIRMED: ESP32-S3 chip ID 9 (GET_SECURITY_INFO)", flush=True)

        # Read-only eFuse indicators. For S3, CRYPT_CNT is three bits at [20:18].
        # Odd parity means flash encryption enabled. Secure Boot V2 is bit 20.
        crypt_reg = loader.read_reg(0x60007034)
        secure_reg = loader.read_reg(0x60007038)
        crypt_cnt = (crypt_reg >> 18) & 0x7
        encrypted = bool(crypt_cnt.bit_count() % 2)
        secure_boot = bool(secure_reg & (1 << 20))
        print(f"Security: flash_encryption={encrypted}, secure_boot={secure_boot}", flush=True)
        flags = struct.unpack_from("<I", security_body, 0)[0]
        secure_boot = secure_boot or bool(flags & 0x1)
        print(f"Security-info flags: 0x{flags:08x}", flush=True)
        if encrypted or secure_boot:
            print("WARNING: Protected flash may not be recoverable from this dump; DO NOT reflash plaintext firmware!", flush=True)

        loader.spi_attach()
        stub = stub_flasher_data.STUBS.get("esp32s3")
        if not stub or not loader.upload_stub(stub):
            raise BackupError("ESP32-S3 RAM readback stub could not start; no flash changed")
        # Preserve the known-good nrflash ROM handshake and RAM-stub startup.
        # The buffered reader is needed only once the stub streams multiple
        # flash-data SLIP frames per USB transfer; before this point it can
        # interfere with ROM command response synchronization.
        loader._slip_read_packet = buffered.read_packet
        print("[2/4] RAM readback stub ready; downloading 16 MiB", flush=True)

        sha = hashlib.sha256()
        md5 = hashlib.md5()
        first_header = None
        with partial.open("xb") as f:
            for offset in range(0, SIZE, CHUNK):
                read_verified_chunk(loader, offset, CHUNK, f)
                f.flush()
                # Re-hash only the latest verified chunk using disk bytes.
                fpos = f.tell()
                with partial.open("rb") as inp:
                    inp.seek(offset)
                    chunk = inp.read(CHUNK)
                if len(chunk) != CHUNK or fpos != offset + CHUNK:
                    raise BackupError("Backup write truncated")
                if first_header is None:
                    first_header = chunk[0]
                sha.update(chunk)
                md5.update(chunk)
                if offset % (1024 * 1024) == 0 or offset + CHUNK == SIZE:
                    print(f"  {offset + CHUNK:,} / {SIZE:,} bytes verified", flush=True)
            f.flush()
            os.fsync(f.fileno())
        print("[3/4] Checking full image on disk", flush=True)
        if partial.stat().st_size != SIZE:
            raise BackupError("Backup size mismatch")
        check = hashlib.sha256()
        with partial.open("rb") as f:
            for data in iter(lambda: f.read(1024 * 1024), b""):
                check.update(data)
        if check.digest() != sha.digest():
            raise BackupError("Backup on-disk SHA-256 mismatch")

        os.replace(partial, output)
        manifest = {
            "chip": chip,
            "flash_size_bytes": SIZE,
            "read_start_offset": 0,
            "filename": output.name,
            "sha256": sha.hexdigest(),
            "md5": md5.hexdigest(),
            "boot_header_first_byte_hex": f"0x{first_header:02x}",
            "boot_header_looks_plaintext_valid": first_header == 0xE9,
            "flash_encryption_enabled": encrypted,
            "secure_boot_enabled": secure_boot,
            "chunk_md5_verified": True,
            "verified_backup": True,
            "warning": "Backup may contain Wi-Fi credentials. Keep private. Restore only to same chip and security configuration.",
        }
        metadata = output.with_suffix(".json")
        metadata.write_text(json.dumps(manifest, indent=2) + "\n")
        output.with_suffix(".sha256").write_text(f"{sha.hexdigest()}  {output.name}\n")
        print("[4/4] BACKUP VERIFIED", flush=True)
        print(f"File: {output}", flush=True)
        print(f"SHA256: {sha.hexdigest()}", flush=True)
        print(f"Flash header: 0x{first_header:02x} (expected 0xe9 for ordinary plaintext bootloader)", flush=True)
        if first_header == 0xFF:
            print("WARNING: flash at 0x0 appears erased; this raw backup may not contain a recoverable original bootloader", flush=True)
        if encrypted or secure_boot:
            print("STOP: Security protections enabled; do not flash unsigned/plaintext firmware.", flush=True)
        success = True
    finally:
        try:
            reset.reset_to_app(device)
        except (Exception, KeyboardInterrupt):
            print("Note: reconnect USB or press EN to leave download mode", flush=True)
        if not success:
            print("INCOMPLETE backup (partial file retained for diagnostics); DO NOT FLASH.", flush=True)


def termux_bootstrap(output: Path) -> None:
    import espbridge as eb
    device_path = eb.auto_detect_device()
    print(f"USB device: {device_path}", flush=True)
    print("Requesting Samsung USB permission...", flush=True)
    if not eb.request_permission(device_path):
        raise BackupError("Samsung USB access denied")
    cmd = " ".join((
        shlex.quote(sys.executable), shlex.quote(str(Path(__file__).resolve())),
        "--output", shlex.quote(str(output))
    ))
    log_file = str(output.parent / (output.stem + "-usb.log"))
    # espbridge's tail thread opens this file O_RDONLY immediately, while the
    # USB child creates it asynchronously. Pre-create it to prevent ENOENT.
    with open(log_file, "x", encoding="utf-8") as log:
        log.write("M3rMa1d S3 no-root backup log initialized\n")
    child_pid, child_done, tail_thread = eb.launch_with_fd(
        cmd=cmd,
        device_path=device_path,
        log_file=log_file,
        tail_fn=lambda line: print(line, end="", flush=True),
    )
    try:
        _, status = os.waitpid(child_pid, 0)
        code = os.waitstatus_to_exitcode(status)
    except KeyboardInterrupt:
        import signal
        os.kill(child_pid, signal.SIGTERM)
        _, status = os.waitpid(child_pid, 0)
        code = os.waitstatus_to_exitcode(status)
    finally:
        child_done.set()
        tail_thread.join(timeout=2)
    if code:
        raise BackupError(f"Readback failed (exit {code}); no firmware flashed")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="Unique .bin path (will never overwrite)")
    args = parser.parse_args()
    default_dir = Path.home() / "m3rmaid-s3-backups"
    timestamp = time.strftime("%Y%m%d-%H%M%S", time.localtime())
    output = (args.output or (default_dir / f"{timestamp}-original-16m.bin")).expanduser().resolve()
    if output.suffix.lower() != ".bin":
        parser.error("Backup filename must end in .bin")
    if output.exists() or output.with_name(output.name + ".partial").exists():
        parser.error("Backup output or .partial already exists; refusing overwrite")
    output.parent.mkdir(parents=True, exist_ok=True)
    try:
        import espbridge as eb
        import nrflash
        if eb.detect_backend() == "termux" and "TERMUX_USB_FD" not in os.environ:
            termux_bootstrap(output)
        else:
            backup_to(output)
    except (BackupError, ImportError, OSError, RuntimeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr, flush=True)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
