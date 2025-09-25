// SPDX-License-Identifier: MIT
/**
 * @file hid.hpp
 * @brief Public interface for the HID (Human Interface Device) component
 *
 * The HID component provides a complete USB HID implementation for emulating
 * a Nintendo Switch–compatible gamepad on the ESP32-S3 platform. It integrates
 * with the ESP-IDF and TinyUSB stack and exposes a thread-safe API for
 * external modules.
 *
 * Main features of the HID component:
 *   - Initialization of TinyUSB with custom USB/HID descriptors
 *   - Background FreeRTOS task for HID state polling and report updates
 *   - Support for 14 buttons, an 8-way D-Pad (hat switch), and 2 analog sticks
 *     (X/Y/Z/Rz axes, 8-bit each)
 *   - Console command registration for USB/HID diagnostics
 *   - Safe, blocking API for sending HID reports
 *   - Runtime state tracking: gamepad connection and USB status
 *
 * Author: Mark Vodyanitskiy (@mvodya)
 * Copyright (c) 2025
 * Contact: mvodya@icloud.com
 */

#pragma once

#include "esp_err.h"

namespace HID {

// Setup USB descriptors & initialize USB stack
esp_err_t init();

// Run task for HID handler
esp_err_t init_hid_task();

// Register console commands
esp_err_t cmds_register();

#define ATTRIBUTE_PACKED  __attribute__((packed, aligned(1)))

// HID device report
typedef struct ATTRIBUTE_PACKED {
  // === [Button Input: 14 buttons] ===
  uint16_t buttons;
  // === [D-Pad: Hat switch] ===
  uint8_t dPad;

  // === [Analog sticks: X/Y/Z/Rz axes] ===
  uint8_t leftXAxis;
  uint8_t leftYAxis;
  uint8_t rightXAxis;
  uint8_t rightYAxis;

  uint8_t filler;
} hid_device_report_t;

// Set HID device report
// Thread-safe. Blocks until report is sended
esp_err_t set_hid_report(hid_device_report_t report);


// Get gamepad state
// Thread-safe
bool is_gamepad_connected();

}  // namespace HID