#pragma once

#include <cstdint>
#include "esp_err.h"

namespace NSGamepad {

// Gamepad buttons
enum Buttons : uint16_t {
  Y = 0,
  B,
  A,
  X,
  L,
  R,
  ZL,
  ZR,
  Minus,
  Plus,
  LStick,
  RStick,
  Home,
  Capture,
  Reserved1,
  Reserved2
};

// Dpad directions
enum DpadDirection : uint8_t {
  up = 0,
  up_right,
  right,
  down_right,
  down,
  down_left,
  left,
  up_left,
  centered = 0xF
};

// Update gamepad state (send report to console)
void update();

// Press button
void press(Buttons button, bool update = false);
// Release button
void release(Buttons button, bool update = false);
// Release all buttons
void releaseAll(bool update = false);

// Press and release button
void click(Buttons button, uint16_t delay = 100);

// Set dpad pressed buttons
void dpad(DpadDirection direction, bool update = false);
// Press and release dpad
void dpadClick(DpadDirection direction, uint16_t delay = 100);

// Left stick axis
void leftAxis(uint8_t x, uint8_t y, bool update = false);
// Right stick axis
void rightAxis(uint8_t x, uint8_t y, bool update = false);

// Register console commands
esp_err_t cmds_register();

}  // namespace NSGamepad