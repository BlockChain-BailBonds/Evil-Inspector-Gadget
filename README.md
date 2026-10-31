# 918 Technologies Evil Inspector Gadget

Release target: `v1.0.0` · host: Samsung Galaxy S26 Ultra / Termux ARM64.

This repository contains the phone controller, ESP32-S3 gateway, Flipper Zero external FAP, shared protocol, diagnostics, tests, and reproducible release tooling. Hardware validation is tracked separately from compilation and is reported as `NOT TESTED` until physical evidence is captured.

## Current status

The Termux host audit is recorded in `logs/2026-10-31-host-audit.txt`. The baseline environment is usable for native ARM64 host development. ESP32 and Flipper toolchain availability and physical hardware identity still require verification.

## Commands

```sh
./tools/doctor.sh
python -m venv .venv
.venv/bin/python -m unittest discover -s tests -v
./tools/build.sh
```

No script flashes hardware. Flashing requires a later, explicit authorization gate after build and device identity checks pass.

