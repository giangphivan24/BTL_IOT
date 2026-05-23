#ifndef __LED_BLINKY__
#define __LED_BLINKY__
#include <Arduino.h>
#include "global.h"

#define LED_GPIO 48 // Chân GPIO điều khiển LED onboard trên Yolo UNO

void led_blinky(void *pvParameters); // Nháy LED theo trạng thái Normal/Warning/Critical

#endif