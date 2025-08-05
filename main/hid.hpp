#include "esp_err.h"

namespace HID {

// Setup USB descriptors & initialize USB stack
esp_err_t init();

// Run task for HID handler
esp_err_t init_hid_task();

// Register console commands
esp_err_t cmds_register();

}  // namespace HID