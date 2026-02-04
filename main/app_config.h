#pragma once

#include <stdint.h>
#define SENSOR_READ_INTERVAL_MS 3000 // Sensor read interval

extern float temp_threshold;
extern float hum_threshold;
extern uint32_t mqtt_send_interval_ms;

extern char mqtt_pub_topic[128];
extern char mqtt_sub_topic[128];
extern char mqtt_alert_topic[128];
void app_config_init(void);