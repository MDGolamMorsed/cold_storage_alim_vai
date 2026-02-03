#include "network.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "sdkconfig.h"
#include <stdio.h>


static const char *TAG = "WIFI_NETWORK";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool is_connected = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    is_connected = false;
    esp_wifi_connect();
    ESP_LOGI(TAG, "Retrying to connect to the AP");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    is_connected = true;
    ESP_LOGI(TAG, "WiFi Connected");
  }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  ESP_LOGD(TAG, "MQTT Event dispatched: %d", (int)event_id);
}

void network_init(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifdef CONFIG_CONNECTION_TYPE_WIFI
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL,
      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL,
      &instance_got_ip));

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = CONFIG_WIFI_SSID,
              .password = CONFIG_WIFI_PASSWORD,
          },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(TAG, "WiFi Init complete. SSID: %s", CONFIG_WIFI_SSID);
#endif

#if defined(CONFIG_CONNECTION_TYPE_WIFI) && defined(CONFIG_ENABLE_MQTT)
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = CONFIG_MQTT_BROKER_URL,
  };
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                 mqtt_event_handler, NULL);
  esp_mqtt_client_start(mqtt_client);
  ESP_LOGI(TAG, "MQTT Client started. Broker: %s", CONFIG_MQTT_BROKER_URL);
#endif
}

void network_send_data(const sensor_readings_t *readings) {
#if defined(CONFIG_CONNECTION_TYPE_WIFI) && defined(CONFIG_ENABLE_MQTT)

  if (!mqtt_client || !is_connected) {
    ESP_LOGW(TAG, "Cannot send MQTT: Not connected");
    return;
  }

  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"dht_temp\": %.2f, \"dht_hum\": %.2f, \"ds_temp\": %.2f}",
           readings->dht_temp, readings->dht_humidity, readings->ds_temp);

  int msg_id =
      esp_mqtt_client_publish(mqtt_client, CONFIG_MQTT_TOPIC, payload, 0, 1, 0);
  ESP_LOGI(TAG, "MQTT Sent: %s, ID: %d", payload, msg_id);
#else
  ESP_LOGI(TAG, "MQTT Disabled. Data not sent.");
#endif
}

int network_is_connected(void) { return is_connected; }