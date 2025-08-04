#include <stdio.h>

#include "esp_console.h"
#include "esp_log.h"

static const char* TAG = "app";

static int test_cmd(int argc, char** argv) {
  printf("Test command executed!\r\n");
  printf("IDF Version: %s\r\n", esp_get_idf_version());
  return 0;
}

void app_main(void) {
  ESP_LOGI(TAG, "App start");

  esp_console_repl_t* repl = NULL;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

  repl_config.prompt = ">";
  repl_config.max_cmdline_length = 1024;

  esp_console_register_help_command();
  const esp_console_cmd_t cmd_test = {
      .command = "test",
      .help = "Sample test command",
      .hint = NULL,
      .func = &test_cmd,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_test));

  esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

  ESP_ERROR_CHECK(esp_console_start_repl(repl));
}