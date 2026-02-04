#pragma once

#include "sensors.h"

void network_init(void);
void network_send_data(const sensor_readings_t *readings);
void network_send_alert(const char *message);
void network_send_mac(void);
int network_is_connected(void);