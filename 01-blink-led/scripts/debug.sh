#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_ROOT=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)
ELF="${PROJECT_ROOT}/build/01-blink-led.elf"
LOG="${PROJECT_ROOT}/build/openocd.log"

make -C "${PROJECT_ROOT}" build

openocd -f "${PROJECT_ROOT}/scripts/openocd.cfg" >"${LOG}" 2>&1 &
OPENOCD_PID=$!
trap 'kill "${OPENOCD_PID}" 2>/dev/null || true' EXIT INT TERM

sleep 1

arm-none-eabi-gdb "${ELF}" \
    -ex "target extended-remote localhost:3333" \
    -ex "monitor halt"
