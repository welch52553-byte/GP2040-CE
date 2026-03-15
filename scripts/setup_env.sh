#!/bin/bash
# Pico Development Environment Setup Script
# Sets up all required tools for building GP2040-CE firmware for Raspberry Pi Pico
set -e

echo "=== GP2040-CE Pico Development Environment Setup ==="
echo ""

# 1. System dependencies
echo "[1/6] Installing system dependencies..."
sudo apt-get update -qq
sudo apt-get install -y -qq \
    cmake \
    ninja-build \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib \
    build-essential \
    git \
    python3 \
    python3-pip \
    2>/dev/null

# 2. Pico SDK
PICO_SDK_DIR="${PICO_SDK_PATH:-/opt/pico-sdk}"
echo "[2/6] Setting up Pico SDK at ${PICO_SDK_DIR}..."

if [ ! -d "${PICO_SDK_DIR}" ]; then
    sudo mkdir -p "${PICO_SDK_DIR}"
    sudo chown "$(whoami)":"$(whoami)" "${PICO_SDK_DIR}"
    git clone --depth 1 --branch 2.2.0 https://github.com/raspberrypi/pico-sdk.git "${PICO_SDK_DIR}"
    cd "${PICO_SDK_DIR}"
    git submodule update --init --recursive
else
    echo "  Pico SDK already exists at ${PICO_SDK_DIR}"
    cd "${PICO_SDK_DIR}"
    current_tag=$(git describe --tags --exact-match 2>/dev/null || echo "unknown")
    echo "  Current version: ${current_tag}"
fi

# 3. Environment variables
echo "[3/6] Configuring environment variables..."
export PICO_SDK_PATH="${PICO_SDK_DIR}"

if ! grep -q "PICO_SDK_PATH" ~/.bashrc 2>/dev/null; then
    echo "" >> ~/.bashrc
    echo "# Pico SDK" >> ~/.bashrc
    echo "export PICO_SDK_PATH=${PICO_SDK_DIR}" >> ~/.bashrc
    echo "  Added PICO_SDK_PATH to ~/.bashrc"
fi

# 4. Node.js (for web configurator build)
echo "[4/6] Checking Node.js..."
if command -v node &>/dev/null; then
    echo "  Node.js $(node --version) found"
else
    echo "  Installing Node.js via nvm..."
    curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash
    export NVM_DIR="$HOME/.nvm"
    [ -s "$NVM_DIR/nvm.sh" ] && . "$NVM_DIR/nvm.sh"
    nvm install --lts
fi

# 5. Git submodules
echo "[5/6] Initializing git submodules..."
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "${SCRIPT_DIR}")"
cd "${PROJECT_DIR}"

if [ -f ".gitmodules" ]; then
    git submodule update --init --recursive
    echo "  Submodules initialized"
fi

# 6. Verify installation
echo "[6/6] Verifying installation..."
echo ""
echo "=== Environment Verification ==="

check_tool() {
    if command -v "$1" &>/dev/null; then
        version=$($1 --version 2>&1 | head -1)
        echo "  [OK] $1: ${version}"
        return 0
    else
        echo "  [FAIL] $1: not found"
        return 1
    fi
}

ERRORS=0
check_tool cmake || ERRORS=$((ERRORS+1))
check_tool arm-none-eabi-gcc || ERRORS=$((ERRORS+1))
check_tool ninja || ERRORS=$((ERRORS+1))
check_tool git || ERRORS=$((ERRORS+1))
check_tool python3 || ERRORS=$((ERRORS+1))

if [ -d "${PICO_SDK_PATH}" ]; then
    echo "  [OK] PICO_SDK_PATH: ${PICO_SDK_PATH}"
else
    echo "  [FAIL] PICO_SDK_PATH not found"
    ERRORS=$((ERRORS+1))
fi

echo ""
if [ ${ERRORS} -eq 0 ]; then
    echo "=== All tools installed successfully! ==="
    echo ""
    echo "To build the firmware:"
    echo "  cd ${PROJECT_DIR}"
    echo "  ./scripts/build.sh                  # Default Pico build"
    echo "  ./scripts/build.sh PicoMultiADC     # Multi-channel ADC build"
    echo ""
    echo "Or using VS Code:"
    echo "  1. Open the project in VS Code"
    echo "  2. Install recommended extensions (Ctrl+Shift+P -> Extensions: Show Recommended)"
    echo "  3. Use Ctrl+Shift+B to build"
else
    echo "=== Setup completed with ${ERRORS} error(s) ==="
    exit 1
fi
