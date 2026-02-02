#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "sensors.h"
#include "network.h"
#include "gsm_module.h"

static const char *TAG = "MAIN";

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

    // Initialize Modules
    sensors_init();

// Initialize wifi Module
#ifdef CONFIG_CONNECTION_TYPE_WIFI
    network_init();
#endif

// Initialize GSM Module
#ifdef CONFIG_CONNECTION_TYPE_GSM
    gsm_module_init();
    gsm_module_send_sms("msg");
    gsm_module_call_emergency();
    gsm_module_process_data(&temp_threshold, &hum_threshold);

#endif

    // Sensors threshold
    sensor_readings_t readings = {0};
    float temp_threshold = 30.0f; // Default High Threshold
    float hum_threshold = 80.0f;  // Default High Threshold

    // 3. Main Loop
    while (1)
    {
        // Read Sensors
        sensors_read_all(&readings);

        // Print to Serial
        ESP_LOGI(TAG, "Readings -> Temp: %.2f C (Thresh: %.2f), Hum: %.2f %% (Thresh: %.2f)",
                 readings.dht_temp, temp_threshold, readings.dht_humidity, hum_threshold);

#ifdef CONFIG_CONNECTION_TYPE_WIFI
        // Send to wifi Network (if enabled and connected)
        network_send_data(&readings);
#endif

#ifdef CONFIG_CONNECTION_TYPE_GSM
        // Check for incoming SMS/Calls and update thresholds
        gsm_module_process_data(&temp_threshold, &hum_threshold);
#endif

        // Check Thresholds and Notify
        if (readings.dht_temp > temp_threshold || readings.dht_humidity > hum_threshold)
        {
            ESP_LOGW(TAG, "Threshold Exceeded! Sending Notifications...");

            char msg[64];
            snprintf(msg, sizeof(msg), "ALERT: Temp %.2f C, Hum %.2f %%", readings.dht_temp, readings.dht_humidity);

#ifdef CONFIG_CONNECTION_TYPE_GSM
            gsm_module_mqtt_publish("alert/threshold", msg);
#endif

#ifdef CONFIG_CONNECTION_TYPE_WIFI
            gsm_module_send_sms(msg);
#endif
        }

        // Wait 5 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}