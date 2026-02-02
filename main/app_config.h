#pragma once

#define SENSOR_READ_INTERVAL_MS 3000 // Sensor read interval

extern float temp_threshold;
extern float hum_threshold;
extern uint32_t mqtt_send_interval_ms;