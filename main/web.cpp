#include "web.hpp"

#include "esp_err.h"
#include "esp_log.h"

namespace WEB {

static const char* TAG = "app web";

// Setup WiFi connection
void wifi_init_sta(void) {
  ESP_LOGI(TAG, "WiFi STA initialization");
}

// Setup web component
esp_err_t init() {
  ESP_LOGI(TAG, "WEB component initialization");

  wifi_init_sta();

  return ESP_OK;
}

// Register console commands
esp_err_t cmds_register() {
  return ESP_OK;
}

}  // namespace WEB