#include "esp_err.h"

namespace HID {

// Setup USB descriptors & initialize USB stack
esp_err_t init();

// Run task for HID handler
esp_err_t init_hid_task();

// Register console commands
esp_err_t cmds_register();

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