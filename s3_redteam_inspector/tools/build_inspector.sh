#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
[ -x ./tools/build.sh ] || { echo 'Missing verified build wrapper'; exit 1; }
sh tools/build.sh s3_uart_inspector
