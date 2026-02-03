#include "app_config.h"
#include "esp_mac.h"
#include "sdkconfig.h"
#include <stdio.h>
#include "esp_log.h"

float temp_threshold = 30.0f; // Default High Threshold
float hum_threshold = 80.0f;  // Default High Threshold
uint32_t mqtt_send_interval_ms = 60000; // Default 60 seconds

char mqtt_pub_topic[128] = {0};
char mqtt_sub_topic[128] = {0};

void app_config_init(void)
{
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    // MACSTR expands to "%02x:%02x:%02x:%02x:%02x:%02x"
    // MAC2STR(mac) expands to mac[0], mac[1], ..., mac[5]
    // ESP_LOGI("WIFI", "MAC Address: " MACSTR, MAC2STR(mac));
    ESP_LOGI("WIFI", "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0],
             mac[1], mac[2], mac[3], mac[4], mac[5]);

#ifndef CONFIG_MQTT_PUB_TOPIC
#define CONFIG_MQTT_PUB_TOPIC "cold_storage/data"
#endif
#ifndef CONFIG_MQTT_SUB_TOPIC
#define CONFIG_MQTT_SUB_TOPIC "cold_storage/commands"
#endif

    // Format: MAC_ADDRESS/TOPIC
    snprintf(mqtt_pub_topic, sizeof(mqtt_pub_topic), "%02X%02X%02X%02X%02X%02X/%s",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], CONFIG_MQTT_PUB_TOPIC);
    snprintf(mqtt_sub_topic, sizeof(mqtt_sub_topic), "%02X%02X%02X%02X%02X%02X/%s",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], CONFIG_MQTT_SUB_TOPIC);
}