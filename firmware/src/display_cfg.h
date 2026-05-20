#pragma once

#include <Arduino_GFX_Library.h>
#include <XPowersLib.h>
#include <SensorQMI8658.hpp>
#include <Wire.h>

// ---------------------------------------------------------------------------
// Board selection. Exactly one of BOARD_AMOLED_216 / BOARD_AMOLED_18 must be
// defined via -D in platformio.ini. Fall back to the original 2.16" board so
// older build configs keep working.
// ---------------------------------------------------------------------------
#if !defined(BOARD_AMOLED_216) && !defined(BOARD_AMOLED_18)
#define BOARD_AMOLED_216
#endif

#if defined(BOARD_AMOLED_216) && defined(BOARD_AMOLED_18)
#error "Define only one of BOARD_AMOLED_216 / BOARD_AMOLED_18"
#endif

// ===========================================================================
// Waveshare ESP32-S3-Touch-AMOLED-2.16  (CO5300, CST9220, 480×480 square)
// ===========================================================================
#if defined(BOARD_AMOLED_216)

#include <TouchDrvCSTXXX.hpp>

#define LCD_WIDTH      480
#define LCD_HEIGHT     480
#define LCD_IS_SQUARE  1

// QSPI display (CO5300)
#define LCD_CS         12
#define LCD_SCLK       38
#define LCD_SDIO0      4
#define LCD_SDIO1      5
#define LCD_SDIO2      6
#define LCD_SDIO3      7
#define LCD_RESET      2

// Touch (CST9220 via I2C)
#define IIC_SDA        15
#define IIC_SCL        14
#define TP_INT         11
#define TP_RST         2          // shared with LCD_RESET
#define TOUCH_ADDR     0x5A

// AXP2101 PMU
#define AXP2101_ADDR   0x34

// Feature flags
#define BOARD_HAS_TCA9554   0
#define BOARD_TOUCH_CST92XX 1
#define BOARD_TOUCH_FT3168  0
#define BOARD_DISPLAY_CO5300 1
#define BOARD_DISPLAY_SH8601 0
#define BOARD_HAS_BTN_RIGHT 1     // GPIO 18 right-side button
#define BOARD_ROTATE_4WAY   1     // square panel can rotate to any of 4 orientations

// Side buttons
#define BTN_BACK_PIN   0          // left → Space (HID)
#define BTN_FWD_PIN    18         // right → Shift+Tab (HID)

extern Arduino_DataBus *bus;
extern Arduino_OLED    *gfx;       // Arduino_CO5300 instance, accessed via base type
extern TouchDrvCST92xx touch;
extern XPowersPMU      pmu;
extern SensorQMI8658   imu;

// ===========================================================================
// Waveshare ESP32-S3-Touch-AMOLED-1.8  (SH8601, FT3168, 368×448 portrait,
// TCA9554 IO expander manages LCD_RESET + TP_RESET)
// ===========================================================================
#elif defined(BOARD_AMOLED_18)

#define LCD_WIDTH      368
#define LCD_HEIGHT     448
#define LCD_IS_SQUARE  0

// QSPI display (SH8601). Reset is driven by the TCA9554 expander, not a GPIO.
#define LCD_CS         12
#define LCD_SCLK       11
#define LCD_SDIO0      4
#define LCD_SDIO1      5
#define LCD_SDIO2      6
#define LCD_SDIO3      7
// LCD reset bit on the TCA9554 expander
#define LCD_RESET_EXIO 0

// Touch (FT3168 via I2C, reset via TCA9554)
#define IIC_SDA        15
#define IIC_SCL        14
#define TP_INT         21
#define TP_RESET_EXIO  1
#define TOUCH_ADDR     0x38

// AXP2101 PMU and TCA9554 expander on the same I2C bus
#define AXP2101_ADDR   0x34
#define TCA9554_ADDR   0x20

// Feature flags
#define BOARD_HAS_TCA9554   1
#define BOARD_TOUCH_CST92XX 0
#define BOARD_TOUCH_FT3168  1
#define BOARD_DISPLAY_CO5300 0
#define BOARD_DISPLAY_SH8601 1
#define BOARD_HAS_BTN_RIGHT 0     // only BOOT (GPIO 0) and AXP PWR keys
#define BOARD_ROTATE_4WAY   0     // rectangular panel: only 0°/180° make sense

// Side buttons
#define BTN_BACK_PIN   0          // BOOT key → Space (HID); long-press is unused
// No GPIO-18 button on this board; Shift+Tab is fired by a long touch-press
// on the Usage screen (see main.cpp).

extern Arduino_DataBus *bus;
extern Arduino_OLED    *gfx;       // Arduino_SH8601 instance, accessed via base type
extern XPowersPMU      pmu;
extern SensorQMI8658   imu;

#endif  // board select
