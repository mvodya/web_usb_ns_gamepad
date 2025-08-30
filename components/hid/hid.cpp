#include "hid.hpp"

#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "device/usbd.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include "tinyusb.h"

namespace HID {

static const char* TAG = "app hid";

// Gamepad HID descriptor for Nintendo Switch
// 14 buttons, 1 8-way dpad, 2 analog sticks (4 axes)
const uint8_t hid_report_descriptor[] = {
    // === [Device Type Declaration] ===
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP), HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),

    // === [Button Input: 14 buttons] ===
    HID_LOGICAL_MIN(0),                           // Buttons: value range 0 (off)
    HID_LOGICAL_MAX(1),                           //   to 1 (pressed)
    HID_PHYSICAL_MIN(0),                          // Physical value range: 0
    HID_PHYSICAL_MIN(1),                          //   to 1
    HID_REPORT_SIZE(1),                           // Each button uses 1 bit
    HID_REPORT_COUNT(14),                         // 14 buttons total
    HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),        // Button usage page
    HID_USAGE_MIN(0x01),                          // Button 1
    HID_USAGE_MAX(0x0E),                          // Button 14
    HID_INPUT(HID_DATA | HID_VARIABLE |           //
              HID_ABSOLUTE | HID_WRAP_NO |        //
              HID_LINEAR | HID_PREFERRED_STATE |  //
              HID_NO_NULL_POSITION),              //
    HID_REPORT_COUNT(2),                          // Padding to align next fields (2 bits)
    HID_INPUT(HID_CONSTANT | HID_ARRAY |          //
              HID_ABSOLUTE | HID_WRAP_NO |        //
              HID_LINEAR | HID_PREFERRED_STATE |  //
              HID_NO_NULL_POSITION),              //

    // === [D-Pad: Hat switch] ===
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),       // Return to Generic Desktop page
    HID_LOGICAL_MAX(7),                           // Logical: 8 directions (0–7)
    HID_PHYSICAL_MAX_N(315, 2),                   // Physical max = 315 degrees
    HID_REPORT_SIZE(4),                           // Hat uses 4 bits
    HID_REPORT_COUNT(1),                          // One hat switch
    HID_UNIT(0x14),                               // Unit: Rotation (deg)
    HID_USAGE(HID_USAGE_DESKTOP_HAT_SWITCH),      // Usage: Hat switch (D-Pad)
    HID_INPUT(HID_DATA | HID_VARIABLE |           //
              HID_ABSOLUTE | HID_WRAP_NO |        //
              HID_LINEAR | HID_PREFERRED_STATE |  //
              HID_NULL_STATE),                    //

    // === [Padding after D-Pad] ===
    HID_UNIT(0x00),                               // Unit: None
    HID_REPORT_COUNT(1),                          // 1 unit of padding
    HID_INPUT(HID_CONSTANT | HID_ARRAY |          //
              HID_ABSOLUTE | HID_WRAP_NO |        //
              HID_LINEAR | HID_PREFERRED_STATE |  //
              HID_NO_NULL_POSITION),              //

    // === [Analog sticks: X/Y/Z/Rz axes] ===
    HID_LOGICAL_MAX_N(255, 1),                    // Axes value: 0–255
    HID_PHYSICAL_MAX_N(255, 1),                   // Physical value: 0–255
    HID_USAGE(HID_USAGE_DESKTOP_X),               // Usage: X-axis
    HID_USAGE(HID_USAGE_DESKTOP_Y),               //        Y-axis
    HID_USAGE(HID_USAGE_DESKTOP_Z),               //        Z-axis (right stick X)
    HID_USAGE(HID_USAGE_DESKTOP_RZ),              //        Rz-axis (right stick Y)
    HID_REPORT_SIZE(8),                           // Each axis: 8 bits
    HID_REPORT_COUNT(4),                          // 4 axes
    HID_INPUT(HID_DATA | HID_VARIABLE |           //
              HID_ABSOLUTE | HID_WRAP_NO |        //
              HID_LINEAR | HID_PREFERRED_STATE |  //
              HID_NO_NULL_POSITION),              //

    // === [Final padding/alignment byte] ===
    HID_REPORT_SIZE(8),                           // 1 byte
    HID_REPORT_COUNT(1),                          //
    HID_INPUT(HID_CONSTANT | HID_ARRAY |          //
              HID_ABSOLUTE | HID_WRAP_NO |        //
              HID_LINEAR | HID_PREFERRED_STATE |  //
              HID_NO_NULL_POSITION),              //

    // === [End of descriptor] ===
    HID_COLLECTION_END};

// USB Device descriptor
const tusb_desc_device_t device_descriptor = {
    .bLength = sizeof(device_descriptor),       // Size of this descriptor in bytes
    .bDescriptorType = TUSB_DESC_DEVICE,        // Device descriptor type (0x01)
    .bcdUSB = 0x0200,                           // USB specification version (2.00)
    .bDeviceClass = TUSB_CLASS_UNSPECIFIED,     // Base class
    .bDeviceSubClass = 0x00,                    // Subclass (unused)
    .bDeviceProtocol = 0x00,                    // Protocol (unused)
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,  // Max packet size for EP0
    .idVendor = 0x0f0d,                         // Vendor ID (HORI)
    .idProduct = 0x00c1,                        // Product ID (Nintendo Switch gamepad)
    .bcdDevice = 0x0572,                        // Device release number
    .iManufacturer = 0x01,                      // Index of manufacturer string (1)
    .iProduct = 0x02,                           // Index of product string (2)
    .iSerialNumber = 0x00,                      // No serial number string
    .bNumConfigurations = 0x01                  // One configuration
};

// USB String descriptor
const char* hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "mvodya",              // 1: Manufacturer
    "WEB USB NS Gamepad",  // 2: Product
    "123456",              // 3: Serials, should use chip ID
    "WEB USB NS Gamepad",  // 4: HID
};

// USB Configuration descriptor
// 1 config, 1 HID
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0x80, 250),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size,
    // polling interval
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, CFG_TUD_HID_EP_BUFSIZE,
                       10),
};

// TinyUSB HID callback
// Invoked when received GET HID REPORT DESCRIPTOR request
extern "C" uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  ESP_LOGI(TAG, "HID descriptor report invoked");
  // We use only one interface and one HID report descriptor, so we can ignore parameter instance
  return hid_report_descriptor;
}

// TinyUSB HID callback
// Invoked when received GET_REPORT control request
extern "C" uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                          hid_report_type_t report_type, uint8_t* buffer,
                                          uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// TinyUSB HID callback
// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
extern "C" void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                                      hid_report_type_t report_type, uint8_t const* buffer,
                                      uint16_t bufsize) {}

// Gamepad report state
static hid_device_report_t hid_report_state;
SemaphoreHandle_t hid_report_state_mtx;

// Is Gamepad connected?
static bool is_gamepad_connected_state = false;
SemaphoreHandle_t is_gamepad_connected_state_mtx;

// Check is gamepad connected
bool is_gamepad_connected() {
  bool state = false;
  if (xSemaphoreTake(is_gamepad_connected_state_mtx, portMAX_DELAY)) {
    state = is_gamepad_connected_state;
    xSemaphoreGive(is_gamepad_connected_state_mtx);
  }
  return state;
}

// Set gamepad state
inline void set_is_gamepad_connected(bool state) {
  if (xSemaphoreTake(is_gamepad_connected_state_mtx, portMAX_DELAY)) {
    is_gamepad_connected_state = state;
    xSemaphoreGive(is_gamepad_connected_state_mtx);
  }
}

// Report HID semaphore
// unlocks, when HID report is sended
SemaphoreHandle_t report_semaphore;
inline void wait_hid_report() {
  xSemaphoreTake(report_semaphore, pdMS_TO_TICKS(1000));
}

// Task for USB HID report
void hid_handler_task(void*) {
  const TickType_t freq = pdMS_TO_TICKS(10);
  TickType_t last_wake_time = xTaskGetTickCount();
  ESP_LOGI(TAG, "HID handler task runned, pooling tickrate: %d", 10);

  while (1) {
    if (tud_mounted()) {
      if (!is_gamepad_connected() && !tud_suspended()) {
        // For connection we need to trigger some buttons after USB initialization
        if (xSemaphoreTake(hid_report_state_mtx, portMAX_DELAY)) {
          // Clear report
          memset(&hid_report_state, 0x00, sizeof(hid_report_state));
          
          // Set axis to zero
          hid_report_state.leftXAxis = 0x80;
          hid_report_state.leftYAxis = 0x80;
          hid_report_state.rightXAxis = 0x80;
          hid_report_state.rightYAxis = 0x80;

          // Push one button for init
          tud_hid_report(0, &hid_report_state, sizeof(hid_report_state));
          vTaskDelay(pdMS_TO_TICKS(1000));
          hid_report_state.buttons = (uint16_t)1;
          tud_hid_report(0, &hid_report_state, sizeof(hid_report_state));
          vTaskDelay(pdMS_TO_TICKS(100));
          hid_report_state.buttons = (uint16_t)0;
          tud_hid_report(0, &hid_report_state, sizeof(hid_report_state));
          vTaskDelay(pdMS_TO_TICKS(100));
          xSemaphoreGive(hid_report_state_mtx);
        }

        ESP_LOGI(TAG, "Gamepad connected");
        set_is_gamepad_connected(true);

      } else if (is_gamepad_connected_state && tud_suspended()) {
        ESP_LOGI(TAG, "Gamepad unconnected");
        set_is_gamepad_connected(false);
      }

      // Report gamepad state
      if (xSemaphoreTake(hid_report_state_mtx, portMAX_DELAY)) {
        tud_hid_report(0, &hid_report_state, sizeof(hid_report_state));
        xSemaphoreGive(hid_report_state_mtx);
      }
      xSemaphoreGive(report_semaphore);
    }
    vTaskDelayUntil(&last_wake_time, freq);
  }
}

// Setup USB descriptors & initialize USB stack
esp_err_t init() {
  ESP_LOGI(TAG, "USB initialization");

  // Create semaphore for HID task
  report_semaphore = xSemaphoreCreateBinary();
  // Create mutex for HID report
  hid_report_state_mtx = xSemaphoreCreateMutex();
  // Create mutex for gamepad state
  is_gamepad_connected_state_mtx = xSemaphoreCreateMutex();

  // TinyUSB config
  const tinyusb_config_t tusb_cfg = {
      .device_descriptor = &device_descriptor,
      .string_descriptor = hid_string_descriptor,
      .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
      .external_phy = false,
      .configuration_descriptor = hid_configuration_descriptor,
  };
  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
  ESP_LOGI(TAG, "USB initialization DONE");

  return ESP_OK;
}

// Run task for HID handler
esp_err_t init_hid_task() {
  ESP_LOGI(TAG, "Create HID handler task");
  xTaskCreate(hid_handler_task, "app_hid_task", 2500, NULL, 6, NULL);

  return ESP_OK;
}

// CMD: Prints USB information
static int cmd_usbinfo(int argc, char** argv) {
  printf("USB info:\r\n");
  printf("  Device HID name: %s\r\n", hid_string_descriptor[4]);
  printf("  Device mount state: %s\r\n", tud_mounted() ? "mounted" : "unmounted");
  printf("  Device connection state: %s\r\n", tud_connected() ? "connected" : "unconnected");
  printf("  Device suspension state: %s\r\n", tud_suspended() ? "suspended" : "not suspended");
  printf("  Gamepad connected: %s\r\n", is_gamepad_connected() ? "true" : "false");
  printf("  Pooling tickrate: %d\r\n", 10);
  return 0;
}

// Register console commands
esp_err_t cmds_register() {
  ESP_LOGI(TAG, "Register console commands");

  const esp_console_cmd_t cmd_usbinfo_cfg = {
      .command = "usbinfo",
      .help = "Get USB status information",
      .hint = NULL,
      .func = &cmd_usbinfo,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_usbinfo_cfg));

  return ESP_OK;
}

// Set HID device report
esp_err_t set_hid_report(hid_device_report_t report) {
  if (is_gamepad_connected()) {
    if (xSemaphoreTake(hid_report_state_mtx, portMAX_DELAY)) {
      hid_report_state = report;
      xSemaphoreGive(hid_report_state_mtx);
    }
    wait_hid_report();
    return ESP_OK;
  }
  return ESP_ERR_INVALID_STATE;
}

}  // namespace HID