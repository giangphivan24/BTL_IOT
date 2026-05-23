#ifndef __LCD_DISPLAY__
#define __LCD_DISPLAY__

#include <Arduino.h>
#include "global.h"

void lcd_display_task(void *pvParameters); // Đọc dữ liệu từ SystemState → hiển thị lên LCD I2C

#endif