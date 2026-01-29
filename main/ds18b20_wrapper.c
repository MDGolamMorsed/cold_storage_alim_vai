#include "ds18b20_wrapper.h"
#include "onewire_bus.h"
#include "ds18b20.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static const char *TAG = "DS18B20_WRAPPER";

static onewire_bus_handle_t bus_handle = NULL;
static ds18b20_device_handle_t ds18b20_handle = NULL;

void ds18b20_wrapper_init(gpio_num_t pin)
{
    // 1. Install 1-Wire Bus (RMT)
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = pin,
        .flags = {
            .en_pull_up = true, // Enable internal pull-up as per ds.md
        },
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10,
    };
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus_handle));

    // 2. Scan for the first device on the bus
    onewire_device_iter_handle_t iter = NULL;
    onewire_new_device_iter(bus_handle, &iter);
    onewire_device_t next_onewire_device;

    if (onewire_device_iter_get_next(iter, &next_onewire_device) == ESP_OK)
    {
        ds18b20_config_t ds_cfg = {}; // Use default configuration
        // Use ds18b20_new_device_from_enumeration based on ds.md
        if (ds18b20_new_device_from_enumeration(&next_onewire_device, &ds_cfg, &ds18b20_handle) == ESP_OK)
        {
            ESP_LOGI(TAG, "DS18B20 device found and initialized");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to create DS18B20 device handle");
        }
    }
    else
    {
        ESP_LOGW(TAG, "No DS18B20 sensor found on bus");
    }
    onewire_del_device_iter(iter);
}

float ds18b20_wrapper_read(void)
{
    if (ds18b20_handle == NULL)
    {
        return (float)CONFIG_DS18B20_FALLBACK_TEMP;
    }

    // Trigger conversion
    ds18b20_trigger_temperature_conversion_for_all(bus_handle);

    // Wait for conversion (750ms for 12-bit)
    vTaskDelay(pdMS_TO_TICKS(800));

    float temperature;
    if (ds18b20_get_temperature(ds18b20_handle, &temperature) != ESP_OK)
    {
        return (float)CONFIG_DS18B20_FALLBACK_TEMP;
    }

    return temperature;
}