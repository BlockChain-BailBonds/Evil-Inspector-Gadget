#!/bin/sh
set -eu
cd "$(dirname "$0")"
PYTHONDONTWRITEBYTECODE=1 python3 test_project.py
PYTHONDONTWRITEBYTECODE=1 python3 test_gpio_integration.py
mkdir -p .host-build
cc -std=c11 -Wall -Wextra -Werror -pedantic \
 -I flipper_fap/inspector_gadget \
 flipper_fap/inspector_gadget/inspector_link.c tests/test_link.c \
 -o .host-build/test_link
.host-build/test_link
(cd s3_redteam_inspector && PYTHONDONTWRITEBYTECODE=1 sh tools/test.sh)
