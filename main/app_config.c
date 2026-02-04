#include "app_config.h"
#include "esp_mac.h"
#include "sdkconfig.h"
#include <stdio.h>
#include "esp_log.h"

threshold_config_t temp_thresh_cfg = { .op = THRESH_GT, .val1 = 30.0f, .val2 = 0.0f };
threshold_config_t hum_thresh_cfg = { .op = THRESH_GT, .val1 = 70.0f, .val2 = 0.0f };
uint32_t mqtt_send_interval_ms = 6000; // Default 60 seconds

char mqtt_pub_topic[128] = {0};
char mqtt_sub_topic[128] = {0};
char mqtt_alert_topic[128] = {0};

void app_config_init(void)
{
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    // MACSTR expands to "%02x:%02x:%02x:%02x:%02x:%02x"
    // MAC2STR(mac) expands to mac[0], mac[1], ..., mac[5]
    // ESP_LOGI("WIFI", "MAC Address: " MACSTR, MAC2STR(mac));
    ESP_LOGI("WIFI", "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0],
             mac[1], mac[2], mac[3], mac[4], mac[5]);

#ifdef CONFIG_ENABLE_MQTT
    // Format: MAC_ADDRESS/TOPIC_BASE
    snprintf(mqtt_pub_topic, sizeof(mqtt_pub_topic), "%02X%02X%02X%02X%02X%02X/%s",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], CONFIG_MQTT_PUB_TOPIC_BASE);
    snprintf(mqtt_sub_topic, sizeof(mqtt_sub_topic), "%02X%02X%02X%02X%02X%02X/%s",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], CONFIG_MQTT_SUB_TOPIC_BASE);
    snprintf(mqtt_alert_topic, sizeof(mqtt_alert_topic), "%02X%02X%02X%02X%02X%02X/%s",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], CONFIG_MQTT_ALERT_TOPIC_BASE);
#endif
}