#pragma once

#include "device.h"

void print_boot_msg(void);
// Checked once per frame from the boot splash and the cart list -- powers
// off and never returns if <START> is currently pressed. See its own
// comment in menu.cpp for why it's scoped to just those two screens, and
// why it's power-off only, not a "return to loader" action.
void HandlePowerOffShortcut(void);
void WaitPress(uint32_t KEY);
void menu_lvl1(flashcart_core::Flashcart* cart);
void menu_lvl2(flashcart_core::Flashcart* cart);
bool d0k3_buttoncombo(int cur_r);
void d0k3_buttoncombo_print_chars(int collumn, int row, uint16_t color, char character);
