# Project context

ESP32-S3 firmware for a desk-side Claude Code usage monitor. Two Waveshare boards supported, selected at build time:

- **ESP32-S3-Touch-AMOLED-2.16** (`-DBOARD_AMOLED_216`) — 480×480 square AMOLED, CO5300 + CST9220
- **ESP32-S3-Touch-AMOLED-1.8**  (`-DBOARD_AMOLED_18`)  — 368×448 portrait AMOLED, SH8601 + FT3168 + TCA9554 IO expander

Connects to a host daemon over BLE; daemon polls Anthropic API for usage data.

This file is for future Claude Code sessions to bootstrap quickly. Read this first.

## Hardware (critical pins)

### Shared
- PMU: **AXP2101** via I2C (addr=0x34) — battery, USB VBUS, PWR button IRQ
- IMU: **QMI8658** via I2C (addr=0x6B) — accelerometer for auto-rotation
- I2C bus: SDA=15, SCL=14

### 2.16" board only
- Display: **CO5300** via QSPI (CS=12, SCLK=38, SDIO0..3=4..7, RST=2)
- Touch: **CST9220** via I2C (INT=11, RST=2 shared with LCD, addr=0x5A)
- Buttons: GPIO 0 (left → Space), GPIO 18 (right → Shift+Tab), AXP PKEY (cycle)

### 1.8" board only
- Display: **SH8601** via QSPI (CS=12, **SCLK=11**, SDIO0..3=4..7, RST via TCA9554 EXIO0)
- Touch: **FT3168** via I2C (INT=21, RST via TCA9554 EXIO1, addr=0x38)
- IO expander: **TCA9554** at addr 0x20 — drives LCD_RESET (EXIO0), TP_RESET (EXIO1), DSI_PWR_EN (EXIO2)
- Buttons: GPIO 0 (BOOT → Space), AXP PKEY (cycle). No right-side button — Shift+Tab is fired by a 500ms long touch-press on the Usage screen instead.
- Rotation is restricted to 0°/180° (rectangular panel can't fit 90°/270°).

## Architecture

```text
main.cpp        — setup(), loop(), button polling (left→Space, right→Shift+Tab, mid→cycle), rotation flash
display_cfg.h   — pin defines, extern object decls
ui.{h,cpp}      — 3-screen UI (splash, usage, bluetooth); splash is touch-toggled, usage↔bluetooth via mid button
splash.{h,cpp}  — 20×20 pixel-art animation engine, 24× upscale to 480×480
imu.{h,cpp}     — accelerometer-driven rotation tracker (returns 0..3)
power.{h,cpp}   — AXP2101 wrapper (battery %, charging, VBUS, PWR button)
touch.{h,cpp}   — minimal tap detector → ui_toggle_splash() (Usage/Splash) or ble_clear_bonds() (BT reset zone)
ble.{h,cpp}     — NimBLE peripheral: custom data service + HID keyboard
data.h          — UsageData struct
icons.h         — icon arrays. Battery (5×) are RGB565A8 with alpha; rest are raw RGB565.
logo.h          — 80×80 RGB565 logo
font_*.c        — pre-compiled LVGL 9 bitmap fonts (Tiempos 56, Styrene 48/28/24/20, Mono 32)
splash_animations.h — generated, do not hand-edit
```

## Build / flash

You must pick a board env. There is no implicit default in `platformio.ini`.

```bash
# 2.16" board
pio run -d firmware -e waveshare_amoled_216
pio run -d firmware -e waveshare_amoled_216 -t upload --upload-port /dev/ttyACM0

# 1.8" board
pio run -d firmware -e waveshare_amoled_18
pio run -d firmware -e waveshare_amoled_18  -t upload --upload-port /dev/ttyACM0
```

`./flash.sh [PORT] [BOARD]` wraps the above (BOARD = `2.16` or `1.8`, default `2.16`).

`/home/hermann/.platformio/penv/bin/pio` if `pio` isn't on PATH.

Device shows up as `/dev/ttyACM0` (Espressif USB JTAG/serial debug unit). No boot-mode gymnastics needed — direct flash works.

## QA your own UI changes — don't ask the user

The firmware ships a `screenshot` serial command that dumps the LVGL framebuffer over `/dev/ttyACM0`. `./screenshot.sh out.png /dev/ttyACM0` captures a 480×480 PNG. **Use this on every UI iteration** — Read the PNG with the Read tool, verify the change visually, iterate.

The boot screen is `SCREEN_SPLASH` and only advances on a physical button press, so a fresh flash will sit on the splash. To screenshot the screen you're actually editing without asking the user to press a button, **temporarily change the default boot screen** in `main.cpp` (search for `ui_show_screen(SCREEN_SPLASH);`) to `SCREEN_USAGE` / `SCREEN_CONTROLLER` / `SCREEN_BLUETOOTH`, do your iteration, then revert before committing.

## Critical gotchas

0. **Board-conditional code.** Pick board with `-DBOARD_AMOLED_216` or `-DBOARD_AMOLED_18` in `platformio.ini`. `display_cfg.h` routes pins, types, and feature flags (`BOARD_DISPLAY_CO5300`, `BOARD_TOUCH_FT3168`, `BOARD_HAS_TCA9554`, `BOARD_ROTATE_4WAY`, `BOARD_HAS_BTN_RIGHT`). `gfx` is typed `Arduino_OLED*` (common base of CO5300/SH8601 — has both `setBrightness` and `draw16bitRGBBitmap`). On the 1.8 board, `expander_init()` + `expander_reset_panel()` must run **before** `gfx->begin()`, otherwise the SH8601 stays in reset.
1. **CO5300 cannot rotate.** Its MADCTL only supports axis flips, not column/row exchange. Rotation is done by **CPU pixel remapping in `my_flush_cb`** in main.cpp. We use **PARTIAL render mode with strip rotation** (small W×40 strips, fast). On rotation change → AMOLED brightness flash → force redraw.
2. **OPI PSRAM** required: `board_build.arduino.memory_type = qio_opi` in platformio.ini. Without this, `MALLOC_CAP_SPIRAM` returns NULL and the screen is black.
3. **pioarduino platform required.** GFX Library for Arduino needs Arduino Core 3.x (`esp32-hal-periman.h`), not the 2.x that standard `espressif32` ships. We pin `pioarduino/platform-espressif32` 55.03.38-1.
4. **LVGL 9 font patching.** `lv_font_conv` outputs LVGL 8 format. Must remove `#if LVGL_VERSION_MAJOR >= 8` guards, drop `.cache` field, add `.release_glyph`, `.kerning`, `.static_bitmap`, `.fallback`, `.user_data`. Without patching, fonts render invisible.
5. **Touch reading must be centralized.** CST9220's `getPoint()` does a full I2C transaction. Calling it from multiple places consumed each other's data and broke input. `touch_read()` is called once per loop in main.cpp; both LVGL `my_touch_cb` and `touch.cpp` read from shared `touch_pressed/touch_x/touch_y` state.
6. **CO5300 needs even-aligned flush regions.** `rounder_cb` enforces this.
7. **Touch `setSwapXY(true)` and `setMirrorXY(true, false)`** are the empirically-correct values for default rotation 0. IMU rotation logic doesn't change touch mapping (it does CPU-side rotation of the rendered pixels, so LVGL still thinks the display is portrait at 0°).
8. **LVGL RGB565A8 is planar.** `w*h` RGB565 pixels followed by `w*h` alpha bytes; `data_size = w*h*3`, `stride = w*2`. Use `init_icon_dsc_rgb565a8()` for icons that overlap non-uniform backgrounds (e.g. battery over splash). Lucide source PNGs are black-on-transparent — converter must tint to white or icons render invisible. See `tools/png_to_lvgl.js`.

## Icons

`tools/png_to_lvgl.js <input.png> <symbol> [W_MACRO] [H_MACRO] [--tint=RRGGBB | --no-tint]` converts an alpha PNG to RGB565A8. Default tint is white (`0xFFFFFF`) — necessary for Lucide PNGs. Splice output into `firmware/src/icons.h` and use `init_icon_dsc_rgb565a8()` in ui.cpp. Currently only the 5 battery icons use this format; the rest are still raw RGB565 baked over the panel background, fine because they live inside opaque zones.

## Splash animations

13 × 20×20 pixel-art creature animations sourced from
[claudepix.vercel.app](https://claudepix.vercel.app). Pipeline:

```bash
node tools/scrape_claudepix.js  # → tools/claudepix_data/*.json
node tools/convert_to_c.js      # → firmware/src/splash_animations.h
```

Each animation has a per-animation 10-color RGB565 palette. Cell values 0..9 index it. Default boot screen.

## User profile / preferences

See `~/.claude/projects/.../memory/` files for persistent context (user is an embedded-beginner senior dev, brand-conscious, prefers iterative UI refinement, dislikes me authoring my own art when third-party assets are intended). Always read those memory files at session start.

## Recent session highlights

- Migrated from Panlee SC01 Plus (480×320 IPS) to Waveshare 2.16" AMOLED (480×480 square). Full hardware/library swap.
- Added IMU auto-rotation, battery indicator, USB-state-aware screen switching.
- Added splash screen with scraped pixel-art animations and 3-button physical input layout.
- Fonts and icons re-scaled ~1.9× for the higher-DPI panel.
- All UI margins widened to 20px to clear the rounded display corners.
- Battery icons converted to RGB565A8 alpha so they blend cleanly over the splash animations.

## Daemon / host side

Bash daemon (`daemon/claude-usage-daemon.sh`) reads OAuth token, polls Anthropic API, sends JSON over BLE GATT. Run with `systemctl --user start claude-usage-daemon`. The unit file's `ExecStart` is the absolute path to the script — repoint it when switching between the worktree and the main checkout.

**Discovery & resilience:**

- Connects by name (`"Claude Controller"`) on first run, caches resolved MAC at `~/.config/claude-usage-monitor/ble-address`. ESP32 BLE addresses are factory-burned per-chip, so swapping any board invalidates the cache.
- On connect failure: cache is dropped AND device is removed from bluez (`bluetoothctl remove`) so the next scan won't re-pick a dead MAC. Multi-candidate scans pick `head -1` and let the failure cycle converge.
- `POLL_INTERVAL=60`, `TICK=5`. Inner loop wakes every 5s to detect disconnects fast; polls Anthropic when 60s elapsed OR when ESP fires a refresh request.

**GATT characteristics on service `4c41555a-...0001`:**

- `...0002` RX — daemon writes JSON usage payload here.
- `...0003` TX — firmware notifies ack/nack (daemon doesn't subscribe).
- `...0004` REQ — firmware fires `0x01` notify in `onSubscribe` if `has_received_data` is false. Daemon subscribes via `setsid bash -c "stdbuf -oL dbus-monitor … | awk …"`; awk drops a flag file the inner loop picks up. See the `feedback_dbus_monitor_pipe` memory for the three subtle gotchas (pipe buffering, busctl-exits race, `wait` blocking on pipeline jobs).
