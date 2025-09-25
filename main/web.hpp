#pragma once

#include "esp_err.h"

namespace WEB {

// Setup web component
esp_err_t init();

// Register console commands
esp_err_t cmds_register();

}  // namespace WEB