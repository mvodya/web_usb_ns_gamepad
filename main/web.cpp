#include "web.hpp"

#include "cJSON.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "freertos/idf_additions.h"
#include "projdefs.h"

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

struct web_server_ctx {
  int todo;
};

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
      vTaskDelay(pdMS_TO_TICKS(200));
      esp_wifi_connect();
      wifi_connect_retry_num++;
      ESP_LOGI(TAG, "Retry to connect to the AP (%i)", wifi_connect_retry_num);
    } else {
      // Faild to connect
      ESP_LOGE(TAG, "Failed to connect to the AP SSID: %s", CONFIG_NSG_WIFI_SSID);
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
esp_err_t wifi_init_sta() {
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

  wifi_config_t wifi_config = {};
  memset(&wifi_config, 0, sizeof(wifi_config_t));
  strncpy((char*)wifi_config.sta.ssid, CONFIG_NSG_WIFI_SSID, sizeof(wifi_config.sta.ssid));
  strncpy((char*)wifi_config.sta.password, CONFIG_NSG_WIFI_PASSWORD,
          sizeof(wifi_config.sta.password));
  wifi_config.sta.listen_interval = 10;
  wifi_config.sta.threshold.rssi = -127;
  wifi_config.sta.threshold.authmode = NSG_WIFI_SCAN_AUTH_MODE_THRESHOLD;
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
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

  return ESP_OK;
}

// API: Test ping API
esp_err_t api_rest_ping(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/json");
  cJSON* root = cJSON_CreateObject();

  cJSON_AddStringToObject(root, "answer", "pong");
  const char* data = cJSON_Print(root);
  httpd_resp_sendstr(req, data);
  free((void*)data);
  cJSON_Delete(root);

  return ESP_OK;
}

// Setup HTTP/RESTful server
esp_err_t web_server_init() {
  ESP_LOGI(TAG, "WEB server initialization");

  web_server_ctx server_ctx = {};

  // Setup HTTP server
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "Starting HTTP Server");
  ESP_ERROR_CHECK(httpd_start(&server, &config));

  /* URI handler for fetching temperature data */
  httpd_uri_t temperature_data_get_uri = {
      .uri = "/api/ping", .method = HTTP_GET, .handler = api_rest_ping, .user_ctx = &server_ctx};
  httpd_register_uri_handler(server, &temperature_data_get_uri);

  return ESP_OK;
}

// Task for init & manage web server
void web_task(void*) {
  ESP_LOGI(TAG, "WEB task runned");

  wifi_init_sta();
  web_server_init();

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Setup web component
esp_err_t init() {
  ESP_LOGI(TAG, "WEB component initialization");

  xTaskCreate(web_task, "web_task", 4096, NULL, 3, NULL);

  return ESP_OK;
}

// Register console commands
esp_err_t cmds_register() {
  ESP_LOGI(TAG, "Register console commands");

  return ESP_OK;
}

}  // namespace WEB