#!/usr/bin/env bash
set -euo pipefail

ELF_FILE="${1:-build/firmware.elf}"
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
GDB_BIN="${GDB:-arm-none-eabi-gdb}"

for tool in openocd "${GDB_BIN}"; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "error: ${tool} was not found in PATH" >&2
        exit 127
    fi
done

if [[ -f "${PROJECT_ROOT}/${ELF_FILE}" ]]; then
    ELF_FILE="${PROJECT_ROOT}/${ELF_FILE}"
elif [[ -f "${ELF_FILE}" ]]; then
    ELF_FILE="$(realpath "${ELF_FILE}")"
else
    echo "error: ELF file not found: ${ELF_FILE}" >&2
    exit 1
fi

openocd -f "${SCRIPT_DIR}/openocd.cfg" >"${PROJECT_ROOT}/build/openocd.log" 2>&1 &
OPENOCD_PID=$!
trap 'kill "${OPENOCD_PID}" >/dev/null 2>&1 || true' EXIT INT TERM

sleep 1

"${GDB_BIN}" "${ELF_FILE}" \
    -ex "target extended-remote localhost:3333" \
    -ex "monitor reset halt"
