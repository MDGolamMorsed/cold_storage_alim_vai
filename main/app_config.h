#pragma once

#include <stdint.h>
#define SENSOR_READ_INTERVAL_MS 3000 // Sensor read interval

typedef enum {
    THRESH_GT,      // Alert if value > val1
    THRESH_LT,      // Alert if value < val1
    THRESH_RANGE_IN // Alert if val1 < value < val2
} threshold_op_t;

typedef struct {
    threshold_op_t op;
    float val1;
    float val2;
} threshold_config_t;

extern threshold_config_t temp_thresh_cfg;
extern threshold_config_t hum_thresh_cfg;
extern uint32_t mqtt_send_interval_ms;

extern char mqtt_pub_topic[128];
extern char mqtt_sub_topic[128];
extern char mqtt_alert_topic[128];
void app_config_init(void);