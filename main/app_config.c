#include "app_config.h"
#include "esp_mac.h"
#include "sdkconfig.h"
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

threshold_config_t temp_thresh_cfg = { .op = THRESH_GT, .val1 = 30.0f, .val2 = 0.0f };
threshold_config_t hum_thresh_cfg = { .op = THRESH_GT, .val1 = 70.0f, .val2 = 0.0f };
uint32_t mqtt_send_interval_ms = 6000; // Default 60 seconds

#ifndef CONFIG_TARGET_PHONE_NUMBER
#define CONFIG_TARGET_PHONE_NUMBER ""
#endif
char target_phone_number[32] = CONFIG_TARGET_PHONE_NUMBER;

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

    // Load Config from NVS
    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK)
    {
        size_t required_size = sizeof(target_phone_number);
        if (nvs_get_str(my_handle, "target_phone", target_phone_number, &required_size) == ESP_OK)
        {
            ESP_LOGI("APP_CONFIG", "Loaded target phone number from NVS: %s", target_phone_number);
        }

        size_t sz = sizeof(threshold_config_t);
        if (nvs_get_blob(my_handle, "temp_cfg", &temp_thresh_cfg, &sz) == ESP_OK)
        {
            ESP_LOGI("APP_CONFIG", "Loaded Temp Config from NVS");
        }

        sz = sizeof(threshold_config_t);
        if (nvs_get_blob(my_handle, "hum_cfg", &hum_thresh_cfg, &sz) == ESP_OK)
        {
            ESP_LOGI("APP_CONFIG", "Loaded Hum Config from NVS");
        }
        nvs_close(my_handle);
    }

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