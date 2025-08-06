#include "nsgamepad.hpp"

#include <cstring>

#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"
#include "hid.hpp"

namespace NSGamepad {

static const char* TAG = "app gamepad";

HID::hid_device_report_t hid_report = {};

// Buttons string list
static const char* button_names[] = {"Y",    "B",       "A",         "X",        "L",      "R",
                                     "ZL",   "ZR",      "Minus",     "Plus",     "LStick", "RStick",
                                     "Home", "Capture", "Reserved1", "Reserved2"};
const int button_names_num = 16;

// Update gamepad state (send report to console)
void update() {
  HID::set_hid_report(hid_report);
}

// Press button
void press(Buttons button, bool u) {
  ESP_LOGI(TAG, "Press button %i [%s] (%s)", button, button_names[button], u ? "+upd" : "noupd");
  hid_report.buttons |= (uint16_t)1 << button;

  if (u) {
    update();
  }
}

// Release button
void release(Buttons button, bool u) {
  ESP_LOGI(TAG, "Release button %i [%s] (%s)", button, button_names[button], u ? "+upd" : "noupd");
  hid_report.buttons &= ~((uint16_t)1 << button);

  if (u) {
    update();
  }
}

// Release all buttons
void releaseAll(bool u) {
  ESP_LOGI(TAG, "Release all buttons (%s)", u ? "+upd" : "noupd");
  memset(&hid_report, 0x00, sizeof(hid_report));

  if (u) {
    update();
  }
}

// Args for press & release cmds
static struct {
  struct arg_str* button =
      arg_strn(NULL, NULL,
               "<Y|B|A|X|L|R|ZL|ZR|Minus|Plus|LStick|RStick|Home|Capture|Reserved1|Reserved2|all>",
               1, 16, "Button to press");
  struct arg_end* end = arg_end(20);
} cmd_press_release_args;

// CMD: Press gamepad button
static int cmd_press(int argc, char** argv) {
  // Check argument parse errpr
  int nerrors = arg_parse(argc, argv, (void**)&cmd_press_release_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, cmd_press_release_args.end, argv[0]);
    return 1;
  }

  if (cmd_press_release_args.button->count < 1) {
    printf("No buttons setted\r\n");
    return 1;
  }

  for (int i = 0; i < cmd_press_release_args.button->count; i++) {
    // Search button
    bool pressed = false;
    for (uint16_t b = 0; b < button_names_num; b++) {
      if (strcmp(button_names[b], cmd_press_release_args.button->sval[i]) == 0) {
        // Press button
        press(static_cast<Buttons>(b));
        pressed = true;
        break;
      }
    }
    if (!pressed) {
      printf("Unrecognized button: \"%s\"\r\n", cmd_press_release_args.button->sval[i]);
    }
  }
  // Send new buttons state
  update();

  return 0;
}

// CMD: Release gamepad button
static int cmd_release(int argc, char** argv) {
  // Check argument parse errpr
  int nerrors = arg_parse(argc, argv, (void**)&cmd_press_release_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, cmd_press_release_args.end, argv[0]);
    return 1;
  }

  if (cmd_press_release_args.button->count < 1) {
    printf("No buttons setted\r\n");
    return 1;
  }

  for (int i = 0; i < cmd_press_release_args.button->count; i++) {
    // Search button
    bool released = false;
    for (uint16_t b = 0; b < button_names_num; b++) {
      if (strcmp(button_names[b], cmd_press_release_args.button->sval[i]) == 0) {
        // Release button
        release(static_cast<Buttons>(b));
        released = true;
        break;
      }
    }
    if (!released) {
      if (strcmp("all", cmd_press_release_args.button->sval[i]) == 0) {
        // Release all buttons
        releaseAll();
      } else {
        printf("Unrecognized button: \"%s\"\r\n", cmd_press_release_args.button->sval[i]);
      }
    }
  }
  // Send new buttons state
  update();

  return 0;
}

// CMD: Click gamepad button
static int cmd_click(int argc, char** argv) {
  return 0;
}

// Register console commands
esp_err_t cmds_register() {
  ESP_LOGI(TAG, "Register console commands");

  // Register press command
  const esp_console_cmd_t cmd_press_cfg = {.command = "press",
                                           .help = "Press gamepad button",
                                           .hint = NULL,
                                           .func = &cmd_press,
                                           .argtable = &cmd_press_release_args};
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_press_cfg));

  const esp_console_cmd_t cmd_release_cfg = {.command = "release",
                                             .help = "Release gamepad button",
                                             .hint = NULL,
                                             .func = &cmd_release,
                                             .argtable = &cmd_press_release_args};
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_release_cfg));

  const esp_console_cmd_t cmd_click_cfg = {
      .command = "click",
      .help = "Press & release gamepad button",
      .hint = NULL,
      .func = &cmd_click,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_click_cfg));

  return ESP_OK;
}
}  // namespace NSGamepad