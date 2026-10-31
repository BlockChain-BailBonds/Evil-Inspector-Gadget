#!/usr/bin/env python3
"""Read-only ESP32 family chip identity check for Samsung Termux/no-root USB.

Uses nrflash/espbridge already installed for USB acquisition. Does not
read, write, or erase SPI flash, upload stubs, or modify fuses.
"""
from __future__ import annotations

import os
from pathlib import Path
import shlex
import struct
import sys


def probe() -> int:
    from nrflash import cli as nf
    from nrflash import rom_loader

    device, ep_in, ep_out, reset = nf.acquire_device()
    try:
        read_fn, write_fn = nf.make_read_write(device, ep_in, ep_out)
        loader = rom_loader.RomLoader(read_fn, write_fn, "esp32")
        print("[*] Entering read-only ROM identification mode", flush=True)
        if not nf.enter_bootloader_and_sync(device, loader, reset):
            print("[ERROR] ROM synchronization failed; hardware identity UNKNOWN", flush=True)
            return 2
        # ESP32-S3 (chip id 9) is identified from GET_SECURITY_INFO (0x14),
        # not from 0x40001000 magic. The latter is legacy/fallback only.
        try:
            _value, body = loader._command(0x14, b"", timeout=2.0, retries=1)
            if len(body) >= 20:
                chip_id = struct.unpack_from("<I", body, 12)[0]
                print(f"GET_SECURITY_INFO chip_id={chip_id} (0x{chip_id:08X})", flush=True)
                if chip_id == 9:
                    print("CONFIRMED: ESP32-S3 (chip ID 9)", flush=True)
                    return 0
                print("NOT ESP32-S3: chip ID differs; do not flash S3 firmware", flush=True)
                return 3
            print(f"[WARN] Security-info response length {len(body)} (chip ID unavailable)", flush=True)
        except Exception as exc:
            print(f"[INFO] GET_SECURITY_INFO unavailable: {type(exc).__name__}", flush=True)
        try:
            magic = loader.read_reg(0x40001000)
            print(f"Legacy magic register 0x40001000 = 0x{magic:08X}", flush=True)
            if magic == 0x00F01D83:
                print("LIKELY classic ESP32; S3 not confirmed. Do not flash S3 firmware.", flush=True)
                return 3
        except Exception as exc:
            print(f"[WARN] Legacy register read failed: {type(exc).__name__}", flush=True)
        print("UNKNOWN CHIP: do not flash; collect boot ROM banner", flush=True)
        return 4
    finally:
        try:
            reset.reset_to_app(device)
        except Exception as exc:
            print(f"[WARN] App reset not confirmed ({type(exc).__name__}); unplug/replug USB", flush=True)


def main() -> int:
    try:
        import espbridge as eb
        if eb.detect_backend() == "termux" and not os.environ.get("TERMUX_USB_FD"):
            path = eb.auto_detect_device()
            print("USB path:", path, flush=True)
            if not eb.request_permission(path):
                print("USB permission denied", flush=True)
                return 1
            output = Path.home() / "m3rmaid-s3-chipid.log"
            # Pre-create log so espbridge's tail thread cannot race the USB child.
            output.touch(exist_ok=True)
            cmd = " ".join((shlex.quote(sys.executable), shlex.quote(str(Path(__file__).resolve()))))
            pid, done, tail = eb.launch_with_fd(
                cmd=cmd, device_path=path, log_file=str(output),
                tail_fn=lambda line: print(line, end="", flush=True)
            )
            try:
                _, status = os.waitpid(pid, 0)
                code = os.waitstatus_to_exitcode(status)
            finally:
                done.set()
                tail.join(timeout=2)
            print("Log:", output, flush=True)
            return code
        return probe()
    except Exception as exc:
        print(f"[ERROR] {type(exc).__name__}: {exc}", flush=True)
        return 1


if __name__ == "__main__":
    sys.exit(main())
