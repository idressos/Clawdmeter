#!/bin/bash
# Build and flash Clawdmeter firmware on macOS.
# Usage:
#   ./flash-mac.sh                              # 2.16" board, auto-detect port
#   ./flash-mac.sh /dev/cu.usbmodem1101         # explicit USB port, 2.16" board
#   ./flash-mac.sh /dev/cu.usbmodem1101 1.8     # 1.8" board (368×448 SH8601/FT3168)
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="$1"
BOARD="${2:-2.16}"

if [ -z "$PORT" ]; then
    PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)
    if [ -z "$PORT" ]; then
        echo "Error: no /dev/cu.usbmodem* device found. Plug in via USB-C."
        exit 1
    fi
fi

case "$BOARD" in
    2.16|216) ENV=waveshare_amoled_216 ;;
    1.8|18)   ENV=waveshare_amoled_18  ;;
    *)
        echo "Error: BOARD must be 2.16 or 1.8 (got '$BOARD')"
        exit 1
        ;;
esac

if ! command -v pio >/dev/null; then
    echo "Error: 'pio' not found. Install with:"
    echo "  brew install platformio"
    exit 1
fi

echo "=== Flashing Clawdmeter ==="
echo "Board: $BOARD ($ENV)"
echo "Port:  $PORT"
echo ""

cd "$SCRIPT_DIR/firmware"
pio run -e "$ENV" -t upload --upload-port "$PORT"

echo ""
echo "=== Done ==="
echo "Monitor with: pio device monitor -p $PORT -b 115200"
