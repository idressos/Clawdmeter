#pragma once

#include "display_cfg.h"

// TCA9554 IO expander wrapper. On boards that don't use one (the 2.16),
// these become no-ops so callers don't need conditional compilation.
//
// On the 1.8 board, EXIO bits we drive are:
//   0 = LCD_RESET   (active low)
//   1 = TP_RESET    (active low)
//   2 = DSI_PWR_EN  (panel rail enable, active high)

bool expander_init(void);
void expander_reset_panel(void);   // pulse LCD_RESET + TP_RESET, raise DSI_PWR_EN
