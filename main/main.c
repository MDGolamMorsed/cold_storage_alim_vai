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

    // 2. Initialize Modules
    sensors_init();
    network_init();
    gsm_module_init();

    sensor_readings_t readings = {0};
    float temp_threshold = 30.0f; // Default High Threshold
    float hum_threshold = 80.0f;  // Default High Threshold

    // 3. Main Loop
    while (1)
    {
        // Check for incoming SMS/Calls and update thresholds
        gsm_module_process_data(&temp_threshold, &hum_threshold);

        // Read Sensors
        sensors_read_all(&readings);

        // Print to Serial
        ESP_LOGI(TAG, "Readings -> Temp: %.2f C (Thresh: %.2f), Hum: %.2f %% (Thresh: %.2f)",
                 readings.dht_temp, temp_threshold, readings.dht_humidity, hum_threshold);

        // Send to Network (if enabled and connected)
        network_send_data(&readings);

        // Check Thresholds and Notify
        if (readings.dht_temp > temp_threshold || readings.dht_humidity > hum_threshold)
        {
            ESP_LOGW(TAG, "Threshold Exceeded! Sending Notifications...");

            char msg[64];
            snprintf(msg, sizeof(msg), "ALERT: Temp %.2f C, Hum %.2f %%", readings.dht_temp, readings.dht_humidity);

            gsm_module_send_sms("+8801521475412", msg);
            gsm_module_mqtt_publish("alert/threshold", msg);
            gsm_module_call_emergency();
        }

        // Wait 5 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}