/**
 * @file lcd_display.cpp
 * @brief Task hiển thị thông tin lên LCD 16x2 I2C.
 *        3 trạng thái hiển thị: Normal (hiển thị OK), Warning (cảnh báo AI hoặc THI),
 *        Critical (nháy màn hình kèm tắt backlight).
 */

#include "lcd_display.h"
#include <LiquidCrystal_I2C.h>

void lcd_display_task(void *pvParameters) {
    SystemState *state = (SystemState *)pvParameters;
    
    LiquidCrystal_I2C lcd(33, 16, 2); 
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("BTL IOT - HCMUT "); // Màn hình khởi động
    lcd.setCursor(0, 1);
    lcd.print("                ");

    float localTemp = 0;
    float localHumi = 0;
    int localDispState = 0;
    bool localAnomaly = false;
    bool toggle = false; // Biến đổi trạng thái nháy cho chế độ Critical

    while (1) {
        // Chờ semaphore từ sensor task hoặc timeout 6 giây (tránh treo nếu cảm biến lỗi)
        xSemaphoreTake(state->lcdReadySync, pdMS_TO_TICKS(6000));

        // Đọc dữ liệu an toàn qua Mutex
        if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            localTemp = state->temperature;
            localHumi = state->humidity;
            localDispState = state->displayState;
            localAnomaly = state->tinyML_Anomaly;
            xSemaphoreGive(state->dataMutex);
        }

        lcd.setCursor(0, 0);
        
        if (localDispState == 0) {
            // Normal: hiển thị nhiệt độ, độ ẩm + "STATUS:OK"
            lcd.printf("T:%4.1fC H:%2.0f%%   ", localTemp, localHumi);
            lcd.setCursor(0, 1);
            lcd.print("   STATUS:OK    "); 
            lcd.backlight();
        } 
        else if (localDispState == 1) {
            // Warning: hiển thị nguồn cảnh báo (AI anomaly hoặc THI cao)
            lcd.printf("T:%4.1fC H:%2.0f%%   ", localTemp, localHumi);
            lcd.setCursor(0, 1);
            lcd.printf("%s", localAnomaly ? " AI ANOMALY !!  " : "   WARNING !!   ");
            lcd.backlight();
        } 
        else {
            // Critical: nháy toàn bộ màn hình (bật/tắt backlight) để thu hút sự chú ý
            toggle = !toggle;
            if (toggle) {
                lcd.printf("T:%4.1fC H:%2.0f%%   ", localTemp, localHumi);
                lcd.setCursor(0, 1);
                lcd.print(" !! CRITICAL !! "); 
                lcd.backlight();
            } else {
                lcd.print("                ");
                lcd.setCursor(0, 1);
                lcd.print("                ");
                lcd.noBacklight(); // Tắt đèn nền để tạo hiệu ứng nháy
            }
        }

        // Critical nháy nhanh hơn (250ms) so với các trạng thái khác (500ms)
        TickType_t renderDelay = (localDispState == 2) ? 250 : 500;
        vTaskDelay(pdMS_TO_TICKS(renderDelay));
    }
}