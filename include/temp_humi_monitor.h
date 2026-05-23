#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__
#include <Arduino.h>
#include "global.h"

void temp_humi_monitor(void *pvParameters); // Đọc DHT20 → lọc Moving Average → tính THI → cập nhật SystemState

#endif