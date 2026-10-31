#!/bin/sh
set -eu
cd "$(dirname "$0")/inspector_gadget"
command -v ufbt >/dev/null 2>&1 || { echo 'ufbt unavailable; install official Flipper build tools on supported host'; exit 2; }
ufbt
find dist -type f -name '*.fap' -print
