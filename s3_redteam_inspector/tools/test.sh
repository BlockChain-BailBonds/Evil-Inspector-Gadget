#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
CXX="${CXX:-c++}"
EXE="${TMPDIR:-/tmp}/m3rmaid-s3-rules-$$"
trap 'rm -f "$EXE"' EXIT HUP INT TERM
"$CXX" -std=c++11 -Wall -Wextra -Werror -pedantic -I src tests/test_rules.cpp -o "$EXE"
"$EXE"
python3 -m unittest discover -s tests -p 'test_*.py' -v
python3 tools/check_layout.py
