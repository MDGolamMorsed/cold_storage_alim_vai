#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "sensors.h"
#include "network.h"
#include "gsm_module.h"
#include "app_config.h"

static const char *TAG = "MAIN";

static sensor_readings_t readings = {0};

#ifdef CONFIG_CONNECTION_TYPE_GSM
void gsm_processing_task(void *pvParameters)
{
    while (1)
    {
        gsm_module_process_data(&temp_threshold, &hum_threshold, &readings);
    }
}
#endif

void app_main(void)
{
    // 1. Initialize NVS (Required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Config (Generate MQTT Topic with MAC)
    app_config_init();

    // Initialize Modules
    sensors_init();

// Initialize wifi Module
#ifdef CONFIG_CONNECTION_TYPE_WIFI
    network_init();

    int retry = 0;
    while (!network_is_connected() && retry < 20)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry++;
    }
    if (network_is_connected())
    {
        network_send_mac();
    }
#endif

// Initialize GSM Module
#ifdef CONFIG_CONNECTION_TYPE_GSM
    gsm_module_init();
    // gsm_module_send_sms("C_S_start");
    vTaskDelay(pdMS_TO_TICKS(5000));

    ret = gsm_module_call_emergency();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Emergency call failed: %s", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Emergency call command sent successfully");
    }

    xTaskCreate(gsm_processing_task, "gsm_task", 4096, NULL, 5, NULL);
#endif

    uint32_t last_mqtt_send_time = 0;
    bool s_is_alert_state = false;

    // 3. Main Loop
    while (1)
    {
        // Read Sensors
        sensors_read_all(&readings);

        // Print to Serial
        ESP_LOGI(TAG, "Readings -> Temp: %.2f C (Thresh: %.2f), Hum: %.2f %% (Thresh: %.2f)",
                 readings.dht_temp, temp_threshold, readings.dht_humidity, hum_threshold);

        // Send Data at defined intervals
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if ((now - last_mqtt_send_time) >= mqtt_send_interval_ms)
        {
#ifdef CONFIG_CONNECTION_TYPE_WIFI
            // Send to wifi Network (if enabled and connected)
            network_send_data(&readings);
#endif
#ifdef CONFIG_CONNECTION_TYPE_GSM
            // Check for incoming SMS/Calls and update thresholds
#ifdef CONFIG_ENABLE_MQTT
            gsm_module_mqtt_publish(&readings);
#endif
#endif
            last_mqtt_send_time = now;
        }

        // Check Thresholds and Notify
        bool is_threshold_exceeded = (readings.dht_temp > temp_threshold || readings.dht_humidity > hum_threshold);

        if (is_threshold_exceeded && !s_is_alert_state)
        {
            ESP_LOGW(TAG, "Threshold Exceeded! Sending Notifications...");
            s_is_alert_state = true;

            char msg[64];
            snprintf(msg, sizeof(msg), "ALERT: Temp %.2f C, Hum %.2f %%", readings.dht_temp, readings.dht_humidity);

#ifdef CONFIG_CONNECTION_TYPE_GSM
            gsm_module_send_alert(msg);
#endif

#ifdef CONFIG_CONNECTION_TYPE_WIFI
            network_send_alert(msg);
#endif
        }
        else if (!is_threshold_exceeded && s_is_alert_state)
        {
            ESP_LOGI(TAG, "Conditions returned to normal. Sending Notification...");
            s_is_alert_state = false;

            char msg[64];
            snprintf(msg, sizeof(msg), "NORMAL: Temp %.2f C, Hum %.2f %%", readings.dht_temp, readings.dht_humidity);

#ifdef CONFIG_CONNECTION_TYPE_GSM
            gsm_module_send_alert(msg);
#endif

#ifdef CONFIG_CONNECTION_TYPE_WIFI
            network_send_alert(msg);
#endif
        }

        // Wait 5 seconds
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}