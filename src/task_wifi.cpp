/**
 * @file task_wifi.cpp
 * @brief Task quản lý kết nối WiFi cho ESP32.
 *        - startAP(): phát WiFi nội bộ (AP mode) để người dùng truy cập Dashboard cấu hình
 *        - wifi_task(): tự động kết nối WiFi STA từ cấu hình đã lưu, retry khi thất bại
 *        ESP32 hoạt động đồng thời AP + STA (WIFI_AP_STA mode).
 */

#include "task_wifi.h"
#include "global.h"
#include <WiFi.h>

const char *MY_AP_SSID = "YOLO_UNO_IOT"; // Tên WiFi AP 
const char *MY_AP_PASS = "12345678";      // Mật khẩu AP 

// Bật Access Point nội bộ: cho phép truy cập Dashboard ngay cả khi chưa có Internet
void startAP() {
    WiFi.softAP(MY_AP_SSID, MY_AP_PASS);
    Serial.printf("Phat WiFi (AP Mode). IP: %s\n", WiFi.softAPIP().toString().c_str());
}

// Tự động kết nối WiFi STA và thông báo cho CoreIoT task khi thành công
void wifi_task(void *pvParameters) {
    SystemState *appState = (SystemState *)pvParameters;
    
    while (1) {
        String ssid = "";
        String pass = "";

        // Đọc SSID/password đã lưu từ SystemState
        if (xSemaphoreTake(appState->dataMutex, portMAX_DELAY) == pdTRUE) {
            ssid = appState->wifiSSID;
            pass = appState->wifiPass;
            xSemaphoreGive(appState->dataMutex);
        }

        // Bỏ qua nếu chưa có SSID hoặc đã kết nối rồi
        if (ssid.isEmpty() || WiFi.status() == WL_CONNECTED) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        Serial.printf("Đang kết nối với Wifi: %s\n", ssid.c_str());
        WiFi.begin(ssid.c_str(), pass.isEmpty() ? NULL : pass.c_str());

        // Chờ tối đa 10 giây (20 lần × 500ms) để kết nối
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            vTaskDelay(pdMS_TO_TICKS(500));
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("\nĐã kết nối WiFi thành công!\nĐịa chỉ IP Local: %s\n", WiFi.localIP().toString().c_str());
            
            if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                appState->hasInternet = true;
                xSemaphoreGive(appState->dataMutex);
            }
            
            // Thông báo CoreIoT task: WiFi đã sẵn sàng, có thể bắt đầu gửi MQTT
            xSemaphoreGive(appState->internetReadySync);
        } else {
            Serial.println("\nKết nối thất bại! Phát lại AP Mode.");
            startAP(); // Đảm bảo AP vẫn hoạt động để người dùng có thể sửa cấu hình
            vTaskDelay(pdMS_TO_TICKS(10000)); // Chờ 10 giây trước khi thử lại
        }
    }
}