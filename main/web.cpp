#include "web.hpp"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "freertos/idf_additions.h"

// Convert option NSG_WIFI_SCAN_AUTH_MODE_THRESHOLD -> wifi_auth_mode_t
#if CONFIG_NSG_WIFI_AUTH_OPEN
#define NSG_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_NSG_WIFI_AUTH_WEP
#define NSG_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_NSG_WIFI_AUTH_WPA_PSK
#define NSG_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_NSG_WIFI_AUTH_WPA2_PSK
#define NSG_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_NSG_WIFI_AUTH_WPA_WPA2_PSK
#define NSG_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_NSG_WIFI_AUTH_WPA3_PSK
#define NSG_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_NSG_WIFI_AUTH_WPA2_WPA3_PSK
#define NSG_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_NSG_WIFI_AUTH_WAPI_PSK
#define NSG_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

namespace WEB {

static const char* TAG = "app web";

// Event group to signal, when WiFi connected
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0  // Connected to the AP with an IP
#define WIFI_FAIL_BIT BIT1       // Failed to connect after the maximum amount of retries

// Number of tries connect to WiFi
static size_t wifi_connect_retry_num = 0;

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                        void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    // First connection
    ESP_LOGI(TAG, "Connecting to %s", CONFIG_NSG_WIFI_SSID);
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    // Connection failed
    if (wifi_connect_retry_num < CONFIG_NSG_WIFI_MAXIMUM_RETRY) {
      // Retry connection
      esp_wifi_connect();
      wifi_connect_retry_num++;
      ESP_LOGI(TAG, "Retry to connect to the AP (%i)", wifi_connect_retry_num);
    } else {
      // Faild to connect
      xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    // Connect established and we got IP
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    ESP_LOGI(TAG, "Got IP:  " IPSTR, IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    // Clear connection tries
    wifi_connect_retry_num = 0;
  }
}

// Setup WiFi connection
void wifi_init_sta_task(void*) {
  ESP_LOGI(TAG, "WiFi STA initialization");

  wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler, NULL, &instance_got_ip));

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = CONFIG_NSG_WIFI_SSID,
              .password = CONFIG_NSG_WIFI_PASSWORD,
              .threshold = {.authmode = NSG_WIFI_SCAN_AUTH_MODE_THRESHOLD},
          },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "WiFi STA initialization DONE");

  // Waiting until connection is established (WIFI_CONNECT_BIT) or connection failed
  // (WIFI_FAIL_BIT). The bits are set by wifi_event_handler()
  EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "Connected to AP SSID: %s", CONFIG_NSG_WIFI_SSID);
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGE(TAG, "Failed to connect to AP SSID: %s", CONFIG_NSG_WIFI_SSID);
  } else {
    ESP_LOGE(TAG, "Unexpected error while WiFi AP connection (SSID: %s)", CONFIG_NSG_WIFI_SSID);
  }

  ESP_LOGI(TAG, "wifi_init_sta_task is finished");
  vTaskDelete(NULL);
}

// Setup web component
esp_err_t init() {
  ESP_LOGI(TAG, "WEB component initialization");

  ESP_LOGI(TAG, "Start WiFi init task");
  xTaskCreate(wifi_init_sta_task, "wifi_init_sta_task", 4096, NULL, 2, NULL);

  return ESP_OK;
}

// Register console commands
esp_err_t cmds_register() {
  ESP_LOGI(TAG, "Register console commands");

  return ESP_OK;
}

}  // namespace WEB