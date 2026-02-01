#include "sensors.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "dht_wrapper.h"
#include "ds18b20_wrapper.h"

static const char *TAG = "SENSORS";

#define DHT_PIN CONFIG_DHT22_GPIO
#define DS_PIN CONFIG_DS18B20_GPIO

void sensors_init(void)
{
    dht_wrapper_init(DHT_PIN);
    ds18b20_wrapper_init(DS_PIN);

    ESP_LOGI(TAG, "Sensors initialized. DHT22: %d, DS18B20: %d", DHT_PIN, DS_PIN);
}

int sensors_read_all(sensor_readings_t *readings)
{
    if (!readings)
        return -1;

    // Read DHT22
    float dht_h = 0, dht_t = 0;
    if (dht_wrapper_read(DHT_PIN, &dht_h, &dht_t) == 0)
    {
        readings->dht_humidity = dht_h;
        readings->dht_temp = dht_t;
    }
    else
    {
        // Keep old values or flag error
        ESP_LOGW(TAG, "Failed to read DHT22");
    }

    // Read DS18B20
    readings->ds_temp = ds18b20_wrapper_read();

    return 0;
}