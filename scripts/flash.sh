#!/usr/bin/env bash
set -euo pipefail

ELF_FILE="${1:-build/*.elf}"
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

if ! command -v openocd >/dev/null 2>&1; then
    echo "error: openocd was not found in PATH" >&2
    exit 127
fi

if [[ ! -f "${PROJECT_ROOT}/${ELF_FILE}" && ! -f "${ELF_FILE}" ]]; then
    echo "error: ELF file not found: ${ELF_FILE}" >&2
    exit 1
fi

if [[ -f "${PROJECT_ROOT}/${ELF_FILE}" ]]; then
    ELF_FILE="${PROJECT_ROOT}/${ELF_FILE}"
else
    ELF_FILE="$(realpath "${ELF_FILE}")"
fi

openocd \
    -f "${SCRIPT_DIR}/openocd.cfg" \
    -c "program ${ELF_FILE} verify reset exit"
