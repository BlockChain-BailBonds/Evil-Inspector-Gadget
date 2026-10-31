#!/usr/bin/env python3
"""Read-only host and toolchain diagnostic for Android/Termux."""
import json, os, platform, shutil, subprocess, sys

TOOLS = ["python", "node", "npm", "git", "clang", "cmake", "ninja", "proot", "proot-distro", "termux-usb", "adb", "gh", "ufbt", "idf.py", "pio"]
def version(cmd):
    try:
        p = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=5)
        return p.stdout.splitlines()[0] if p.stdout else f"exit={p.returncode}"
    except Exception as e: return str(e)
def main():
    result = {"platform": platform.platform(), "machine": platform.machine(), "python": sys.version.split()[0], "cwd": os.getcwd(), "tools": {}}
    for name in TOOLS:
        path = shutil.which(name)
        result["tools"][name] = {"path": path, "version": version([name, "--version"]) if path else None}
    usb = shutil.which("termux-usb")
    if usb: result["usb_devices"] = subprocess.run([usb, "-l"], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.strip()
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0
if __name__ == "__main__": raise SystemExit(main())

