#pragma once

#include <stdint.h>

typedef struct
{
    float dht_temp;
    float dht_humidity;
    float ds_temp;
} sensor_readings_t;

// Initialize sensor GPIOs
void sensors_init(void);

// Read all sensors. Returns 0 on success.
// Populates the struct passed by pointer.
int sensors_read_all(sensor_readings_t *readings);