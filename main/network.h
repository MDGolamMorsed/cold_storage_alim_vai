#pragma once

#include "sensors.h"

void network_init(void);
void network_send_data(const sensor_readings_t *readings);
int network_is_connected(void);