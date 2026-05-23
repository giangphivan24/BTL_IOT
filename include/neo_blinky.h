#ifndef __NEO_BLINKY__
#define __NEO_BLINKY__
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "global.h"

#define NEO_PIN 45   // Chân GPIO kết nối NeoPixel
#define LED_COUNT 1  // Số lượng LED NeoPixel trên dải

void neo_blinky(void *pvParameters); // Đổi màu NeoPixel theo humidity và displayState

#endif