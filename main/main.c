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
    gsm_module_call_emergency();
        // 3. Main Loop
        while (1)
    {
        // Read Sensors
        sensors_read_all(&readings);

        // Print to Serial
        ESP_LOGI(TAG, "Readings -> DHT Temp: %.2f C, DHT Hum: %.2f %%, DS Temp: %.2f C",
                 readings.dht_temp, readings.dht_humidity, readings.ds_temp);

        // Send to Network (if enabled and connected)
        network_send_data(&readings);

        // Wait 5 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}