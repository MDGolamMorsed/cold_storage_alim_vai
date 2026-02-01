#pragma once

#include "driver/gpio.h"

// Initialize the DS18B20 bus and device
void ds18b20_wrapper_init(gpio_num_t pin);

// Read temperature
// Returns temperature in Celsius, or fallback value on error
float ds18b20_wrapper_read(void);