#include "dht_wrapper.h"
#include "dht.h" // Provided by esp-idf-lib/dht
#include "esp_log.h"

static const char *TAG = "DHT_WRAPPER";

void dht_wrapper_init(gpio_num_t pin)
{
    // Ensure pull-up is active for signal integrity
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
}

int dht_wrapper_read(gpio_num_t pin, float *humidity, float *temperature)
{
    // Use the library function to read DHT22
    esp_err_t ret = dht_read_float_data(DHT_TYPE_AM2301, pin, humidity, temperature);

    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "DHT Read Error: %d", ret);
        return -1;
    }
    return 0;
}