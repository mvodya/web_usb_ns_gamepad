#include <stdio.h>

#include "esp_console.h"
#include "esp_log.h"
#include "hid.hpp"

static const char* TAG = "app";

void app(void) {
  ESP_LOGI(TAG, "App start");

  // Init USB
  ESP_ERROR_CHECK(HID::init());
  ESP_ERROR_CHECK(HID::init_hid_task());

  // Init console
  esp_console_repl_t* repl = NULL;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

  repl_config.prompt = ">";
  repl_config.max_cmdline_length = 1024;

  // Register console commands
  esp_console_register_help_command();
  ESP_ERROR_CHECK(HID::cmds_register());

  // Start console
  esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
  ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

extern "C" void app_main(void) {
  app();
}