#!/usr/bin/env bash
set -euo pipefail

# Default serial device (update if yours changes)
PORT="${1:-/dev/cu.usbserial-DM7J45ZR}"

# Path to DK6Programmer.exe
EXE="/Users/hongyilin/projects/location_intelligence_embedded_code/DK6Programmer.exe"

# Default Wine prefix (using ~/.wine)
PREFIX="$HOME/.wine"

# Map COM3 to the Mac serial port
mkdir -p "$PREFIX/dosdevices"
ln -sf "$PORT" "$PREFIX/dosdevices/com3"

echo "✅ Mapped $PORT -> COM3 inside $PREFIX"
echo "🚀 Launching DK6 Programmer..."

# Run the Windows exe
exec wine "$EXE"
