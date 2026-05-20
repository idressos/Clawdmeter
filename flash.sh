#!/bin/bash
# Build and flash Clawdmeter firmware (Linux).
# Usage:
#   ./flash.sh                        # 2.16" board on /dev/ttyACM0
#   ./flash.sh /dev/ttyACM0 2.16      # explicit
#   ./flash.sh /dev/ttyACM0 1.8       # 1.8" board (368×448 SH8601/FT3168)
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${1:-/dev/ttyACM0}"
BOARD="${2:-2.16}"

case "$BOARD" in
    2.16|216) ENV=waveshare_amoled_216 ;;
    1.8|18)   ENV=waveshare_amoled_18  ;;
    *)
        echo "Error: BOARD must be 2.16 or 1.8 (got '$BOARD')"
        exit 1
        ;;
esac

echo "=== Flashing Clawdmeter ==="
echo "Board: $BOARD ($ENV)"
echo "Port:  $PORT"
echo ""

cd "$SCRIPT_DIR/firmware"
~/.platformio/penv/bin/pio run -e "$ENV" -t upload --upload-port "$PORT"

echo ""
echo "=== Done! ==="
