#!/bin/bash
# Build script for GP2040-CE firmware
# Usage: ./scripts/build.sh [BoardConfig] [BuildType]
#   BoardConfig: Pico (default), PicoMultiADC, PicoW, etc.
#   BuildType:   Release (default), Debug, RelWithDebInfo
set -e

BOARD_CONFIG="${1:-Pico}"
BUILD_TYPE="${2:-Release}"
BUILD_DIR="build"
SKIP_WEB="${SKIP_WEBBUILD:-TRUE}"
PICO_SDK="${PICO_SDK_PATH:-/opt/pico-sdk}"

echo "=== GP2040-CE Firmware Build ==="
echo "  Board:      ${BOARD_CONFIG}"
echo "  Build type: ${BUILD_TYPE}"
echo "  SDK path:   ${PICO_SDK}"
echo "  Skip web:   ${SKIP_WEB}"
echo ""

if [ ! -d "${PICO_SDK}" ]; then
    echo "ERROR: Pico SDK not found at ${PICO_SDK}"
    echo "Run ./scripts/setup_env.sh first, or set PICO_SDK_PATH"
    exit 1
fi

# Configure
echo "[1/2] Configuring CMake..."
cmake -B "${BUILD_DIR}" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DPICO_SDK_PATH="${PICO_SDK}" \
    -DGP2040_BOARDCONFIG="${BOARD_CONFIG}" \
    -DSKIP_WEBBUILD="${SKIP_WEB}" \
    -DSKIP_SUBMODULES=TRUE

# Build
echo ""
echo "[2/2] Building firmware..."
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j "$(nproc)"

# Report
echo ""
echo "=== Build Complete ==="
UF2_FILE=$(find "${BUILD_DIR}" -name "*.uf2" -print -quit 2>/dev/null)
if [ -n "${UF2_FILE}" ]; then
    SIZE=$(stat -f%z "${UF2_FILE}" 2>/dev/null || stat -c%s "${UF2_FILE}" 2>/dev/null)
    echo "  Output: ${UF2_FILE}"
    echo "  Size:   ${SIZE} bytes"
    echo ""
    echo "Flash instructions:"
    echo "  1. Hold BOOTSEL button on Pico and plug in USB"
    echo "  2. Copy ${UF2_FILE} to the RPI-RP2 drive"
    echo "  Or on Linux: cp ${UF2_FILE} /media/\$(whoami)/RPI-RP2/"
else
    echo "  WARNING: No .uf2 file found in build directory"
fi
