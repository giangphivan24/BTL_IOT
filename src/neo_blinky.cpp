/**
 * @file neo_blinky.cpp
 * @brief Task điều khiển NeoPixel RGB LED.
 *        Màu sắc phản ánh trạng thái môi trường:
 *        - Normal: màu biến đổi liên tục theo mức độ ẩm (trắng xanh → xanh lá → xanh dương)
 *        - Warning: vàng cố định
 *        - Critical: nháy đỏ nhanh
 *        Hỗ trợ AUTO/MANUAL mode.
 */

#include "neo_blinky.h"

void neo_blinky(void *pvParameters){
    SystemState *state = (SystemState *)pvParameters;

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.setBrightness(150);
    strip.show();

    float localHumi = -1.0f;  // -1 = chưa có dữ liệu từ cảm biến
    int localDispState = 0; 
    bool blinkState = false;

    bool isAuto = true;
    bool localNeoState = false;

    while(1) {  
        // Đọc chế độ Auto/Manual và lệnh bật/tắt từ Dashboard
        if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            isAuto = state->isAutoMode;
            localNeoState = state->neoState;
            xSemaphoreGive(state->dataMutex);
        }

        // MANUAL mode + người dùng tắt → tắt LED hoàn toàn
        if (!isAuto && !localNeoState) {
            strip.setPixelColor(0, strip.Color(0, 0, 0));
            strip.show();
            vTaskDelay(pdMS_TO_TICKS(100));
            continue; 
        }

        // AUTO hoặc MANUAL (khi đã bật): đọc dữ liệu để hiển thị đúng màu
        if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            localHumi = state->humidity;
            localDispState = state->displayState;
            xSemaphoreGive(state->dataMutex);
        }

        // Chưa có dữ liệu cảm biến: hiển thị xanh dương nhạt (trạng thái chờ)
        if (localHumi < 0) {
            strip.setPixelColor(0, strip.Color(0, 0, 50));
            strip.show();
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        
        if (localDispState == 2) {
            // Critical: nháy đỏ nhanh, độ sáng tối đa
            blinkState = !blinkState;
            if (blinkState) {
                strip.setPixelColor(0, strip.Color(255, 0, 0)); 
                strip.setBrightness(255);
            } else {
                strip.setPixelColor(0, strip.Color(0, 0, 0)); 
            }
            strip.show();
            vTaskDelay(pdMS_TO_TICKS(150)); 
        } 
        else if (localDispState == 1) {
            // Warning: vàng cam cố định
            strip.setBrightness(150);
            strip.setPixelColor(0, strip.Color(255, 150, 0)); 
            strip.show();
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        else {
            // Normal: màu biến đổi liên tục theo mức độ ẩm
            int r = 0, g = 255, b = 0;
            
            if (localHumi < 40.0f) {
                // Khô hanh (< 40%): trắng xanh → xanh lá nhạt (ẩm càng cao → càng xanh)
                r = (int)map((long)localHumi, 0L, 40L, 150L, 0L); 
                g = 255; 
                b = (int)map((long)localHumi, 0L, 40L, 150L, 0L);
            } 
            else if (localHumi <= 70.0f) {
                // Lý tưởng (40-70%): xanh lá thuần
                r = 0; 
                g = 255; 
                b = 0;
            } 
            else {
                // Ẩm ướt (> 70%): xanh lá → pha thêm xanh dương (ẩm càng cao → càng xanh dương)
                r = 0; 
                g = 255; 
                b = (int)map((long)localHumi, 70L, 100L, 0L, 255L);
            }

            strip.setBrightness(150);
            strip.setPixelColor(0, strip.Color(r, g, b));
            strip.show();
            vTaskDelay(pdMS_TO_TICKS(200)); 
        }
    }
}