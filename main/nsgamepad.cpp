#include "nsgamepad.hpp"

#include <cstring>

#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "hid.hpp"

namespace NSGamepad {

static const char* TAG = "app gamepad";

HID::hid_device_report_t hid_report = {
    .buttons = 0x0,
    .dPad = 0xF,
    // Set axis to zero
    .leftXAxis = 0x80,
    .leftYAxis = 0x80,
    .rightXAxis = 0x80,
    .rightYAxis = 0x80,
};

// Buttons string list
static const char* button_names[] = {"Y",    "B",       "A",         "X",        "L",      "R",
                                     "ZL",   "ZR",      "Minus",     "Plus",     "LStick", "RStick",
                                     "Home", "Capture", "Reserved1", "Reserved2"};
const int button_names_num = 16;

// Dpad string list
static const char* dpad_names[] = {"U", "UR", "R", "DR", "D", "DL", "L", "UL",
                                   "0", "",   "",  "",   "",  "",   "",  ""};
const int dpad_names_num = 9;

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
  memset(&hid_report.buttons, 0x00, sizeof(hid_report.buttons));

  if (u) {
    update();
  }
}

// Press and release button
void click(Buttons button, uint16_t delay) {
  ESP_LOGI(TAG, "Click button %i [%s], delay: %ims", button, button_names[button], delay);
  press(button, true);
  vTaskDelay(pdMS_TO_TICKS(delay));
  release(button, true);
  vTaskDelay(pdMS_TO_TICKS(delay));
}

// Set dpad direction
void dpad(DpadDirection d, bool u) {
  ESP_LOGI(TAG, "Set dpad direction [%s] (%s)", dpad_names[d], u ? "+upd" : "noupd");
  hid_report.dPad = d;

  if (u) {
    update();
  }
}

// Press and release dpad
void dpadClick(DpadDirection d, uint16_t delay) {
  ESP_LOGI(TAG, "Click dpad in direction [%s], delay: %ims", dpad_names[d], delay);

  dpad(d, true);
  vTaskDelay(pdMS_TO_TICKS(delay));
  dpad(DpadDirection::centered, true);
  vTaskDelay(pdMS_TO_TICKS(delay));
}

// Args for press & release cmds
static struct {
  struct arg_str* button =
      arg_strn(NULL, NULL, "<Y|B|A|X|L|R|ZL|ZR|Minus|Plus|LStick|RStick|Home|Capture|all>", 1, 16,
               "Gamepad button");
  struct arg_end* end = arg_end(20);
} cmd_press_release_args;

// CMD: Press gamepad button
static int cmd_press(int argc, char** argv) {
  // Check argument parse error
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
  // Check argument parse error
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
static struct {
  struct arg_str* button =
      arg_strn(NULL, NULL, "<Y|B|A|X|L|R|ZL|ZR|Minus|Plus|LStick|RStick|Home|Capture>", 1, 16,
               "Gamepad button");
  struct arg_int* delay =
      arg_int0("d", "delay", "<d>", "Delay after press and release, default = 100");
  struct arg_end* end = arg_end(20);
} cmd_click_args;
static int cmd_click(int argc, char** argv) {
  // Check argument parse error
  int nerrors = arg_parse(argc, argv, (void**)&cmd_click_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, cmd_click_args.end, argv[0]);
    return 1;
  }

  if (cmd_click_args.button->count < 1) {
    printf("No buttons setted\r\n");
    return 1;
  }

  // Set delay
  int delay = 100;
  if (cmd_click_args.delay->count == 1) {
    delay = cmd_click_args.delay->ival[0];
  }

  for (int i = 0; i < cmd_click_args.button->count; i++) {
    // Search button
    bool clicked = false;
    for (uint16_t b = 0; b < button_names_num; b++) {
      if (strcmp(button_names[b], cmd_click_args.button->sval[i]) == 0) {
        // Click button
        click(static_cast<Buttons>(b), delay);
        clicked = true;
        break;
      }
    }
    if (!clicked) {
      printf("Unrecognized button: \"%s\"\r\n", cmd_click_args.button->sval[i]);
    }
  }

  return 0;
}

// CMD: Set dpad direction
static struct {
  struct arg_str* direction =
      arg_str0(NULL, NULL, "<U|UR|R|DR|D|DL|L|UL|0>", "Dpad direction, 0 - no direction");
  struct arg_end* end = arg_end(20);
} cmd_setdpad_args;
static int cmd_setdpad(int argc, char** argv) {
  // Check argument parse error
  int nerrors = arg_parse(argc, argv, (void**)&cmd_setdpad_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, cmd_setdpad_args.end, argv[0]);
    return 1;
  }

  if (cmd_setdpad_args.direction->count < 1) {
    printf("No dpad direction setted\r\n");
    return 1;
  }

  // Search direction
  bool setted = false;
  for (uint16_t d = 0; d < dpad_names_num; d++) {
    if (strcmp(dpad_names[d], cmd_setdpad_args.direction->sval[0]) == 0) {
      // Set direction
      dpad(static_cast<DpadDirection>(d), true);
      setted = true;
      break;
    }
  }
  if (!setted) {
    printf("Unrecognized direction: \"%s\"\r\n", cmd_setdpad_args.direction->sval[0]);
  }

  return 0;
}

// CMD: Set & unset dpad direction
static struct {
  struct arg_str* direction =
      arg_strn(NULL, NULL, "<U|UR|R|DR|D|DL|L|UL|0>", 1, 16, "Dpad direction, 0 - no direction");
  struct arg_int* delay =
      arg_int0("d", "delay", "<d>", "Delay after press and release, default = 100");
  struct arg_end* end = arg_end(20);
} cmd_dpad_args;
static int cmd_dpad(int argc, char** argv) {
  // Check argument parse error
  int nerrors = arg_parse(argc, argv, (void**)&cmd_dpad_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, cmd_dpad_args.end, argv[0]);
    return 1;
  }

  if (cmd_dpad_args.direction->count < 1) {
    printf("No dpad direction setted\r\n");
    return 1;
  }

  // Set delay
  int delay = 100;
  if (cmd_dpad_args.delay->count == 1) {
    delay = cmd_dpad_args.delay->ival[0];
  }

  for (int i = 0; i < cmd_dpad_args.direction->count; i++) {
    // Search direction
    bool clicked = false;
    for (uint16_t d = 0; d < dpad_names_num; d++) {
      if (strcmp(dpad_names[d], cmd_dpad_args.direction->sval[i]) == 0) {
        // Set direction
        dpadClick(static_cast<DpadDirection>(d), delay);
        clicked = true;
        break;
      }
    }
    if (!clicked) {
      printf("Unrecognized direction: \"%s\"\r\n", cmd_dpad_args.direction->sval[i]);
    }
  }

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

  // Register release command
  const esp_console_cmd_t cmd_release_cfg = {.command = "release",
                                             .help = "Release gamepad button",
                                             .hint = NULL,
                                             .func = &cmd_release,
                                             .argtable = &cmd_press_release_args};
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_release_cfg));

  // Register click command
  const esp_console_cmd_t cmd_click_cfg = {.command = "click",
                                           .help = "Press & release gamepad button",
                                           .hint = NULL,
                                           .func = &cmd_click,
                                           .argtable = &cmd_click_args};
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_click_cfg));

  // Register setdpad command
  const esp_console_cmd_t cmd_setdpad_cfg = {.command = "setdpad",
                                             .help = "Set dpad direction",
                                             .hint = NULL,
                                             .func = &cmd_setdpad,
                                             .argtable = &cmd_setdpad_args};
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_setdpad_cfg));

  // Register dpad command
  const esp_console_cmd_t cmd_dpad_cfg = {.command = "dpad",
                                          .help = "Set & unset dpad direction",
                                          .hint = NULL,
                                          .func = &cmd_dpad,
                                          .argtable = &cmd_dpad_args};
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_dpad_cfg));

  return ESP_OK;
}
}  // namespace NSGamepad