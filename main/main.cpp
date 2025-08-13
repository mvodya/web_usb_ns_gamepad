#include <stdio.h>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hid.hpp"
#include "nsgamepad.hpp"
#include "nvs_flash.h"
#include "web.hpp"

static const char* TAG = "app";

void app(void) {
  ESP_LOGI(TAG, "App start");

  // Init NVS
  ESP_LOGI(TAG, "Init NVS");
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGI(TAG, "Erase NVS & reinit");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Init USB
  ESP_ERROR_CHECK(HID::init());
  ESP_ERROR_CHECK(HID::init_hid_task());

  // Init WEB (& WiFi)
  ESP_ERROR_CHECK(WEB::init());

  // Init console
  esp_console_repl_t* repl = NULL;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

  repl_config.prompt = ">";
  repl_config.max_cmdline_length = 1024;

  // Register console commands
  esp_console_register_help_command();
  ESP_ERROR_CHECK(HID::cmds_register());
  ESP_ERROR_CHECK(NSGamepad::cmds_register());
  ESP_ERROR_CHECK(WEB::cmds_register());

  // Start console
  esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
  ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

extern "C" void app_main(void) {
  app();
}