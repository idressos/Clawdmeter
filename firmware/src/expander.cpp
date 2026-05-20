#include "expander.h"
#include <Arduino.h>
#include <Wire.h>

#if BOARD_HAS_TCA9554

// TCA9554 registers
#define TCA_REG_INPUT   0x00
#define TCA_REG_OUTPUT  0x01
#define TCA_REG_POLARITY 0x02
#define TCA_REG_CONFIG  0x03   // bit=1 → input, bit=0 → output

#define EXIO_LCD_RESET  (1 << 0)
#define EXIO_TP_RESET   (1 << 1)
#define EXIO_DSI_PWR_EN (1 << 2)
#define EXIO_OUTPUTS    (EXIO_LCD_RESET | EXIO_TP_RESET | EXIO_DSI_PWR_EN)

static uint8_t s_output_cache = 0;

static bool tca_write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(TCA9554_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static bool tca_set_output(uint8_t mask, bool high) {
    if (high) s_output_cache |= mask;
    else      s_output_cache &= ~mask;
    return tca_write(TCA_REG_OUTPUT, s_output_cache);
}

bool expander_init(void) {
    // Probe — if no ACK, the expander isn't on this bus.
    Wire.beginTransmission(TCA9554_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("TCA9554 not found");
        return false;
    }

    // Start with all controlled lines low, configure them as outputs.
    s_output_cache = 0;
    tca_write(TCA_REG_OUTPUT, s_output_cache);
    // CONFIG bit = 0 → output. Other bits stay as inputs (1).
    tca_write(TCA_REG_CONFIG, (uint8_t)~EXIO_OUTPUTS);
    Serial.println("TCA9554 init OK");
    return true;
}

void expander_reset_panel(void) {
    // Hold both panels in reset, drop the DSI rail.
    tca_set_output(EXIO_OUTPUTS, false);
    delay(20);
    // Enable the panel rail first so LCD/TP are powered when their reset
    // releases.
    tca_set_output(EXIO_DSI_PWR_EN, true);
    delay(10);
    // Release both reset lines together.
    tca_set_output(EXIO_LCD_RESET | EXIO_TP_RESET, true);
    delay(120);   // SH8601 needs ~120ms after reset before SLPOUT
}

#else  // ---------------- no expander on this board ----------------

bool expander_init(void) { return true; }
void expander_reset_panel(void) {}

#endif
