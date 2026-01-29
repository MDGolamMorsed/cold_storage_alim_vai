#pragma once

#include "driver/gpio.h"

// Initialize DHT sensor GPIO settings
void dht_wrapper_init(gpio_num_t pin);

// Read humidity and temperature
// Returns 0 on success, -1 on error
int dht_wrapper_read(gpio_num_t pin, float *humidity, float *temperature);