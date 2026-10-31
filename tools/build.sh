#!/data/data/com.termux/files/usr/bin/sh
set -eu
cd "$(dirname "$0")/.."
.venv/bin/python -m unittest discover -s tests -v
mkdir -p dist
.venv/bin/python tools/doctor.py > dist/doctor.json
printf '%s\n' 'Host build completed; physical firmware validation remains NOT TESTED.'

