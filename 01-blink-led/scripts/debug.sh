#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

if [[ $# -ge 1 ]]; then
    ELF_FILE="$1"
else
    mapfile -t ELF_CANDIDATES < <(find "${PROJECT_ROOT}/build" -maxdepth 1 -name '*.elf' 2>/dev/null)
    if [[ ${#ELF_CANDIDATES[@]} -ne 1 ]]; then
        echo "error: expected exactly one .elf file in build/, found ${#ELF_CANDIDATES[@]}; pass the path explicitly" >&2
        exit 1
    fi
    ELF_FILE="${ELF_CANDIDATES[0]}"
fi

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