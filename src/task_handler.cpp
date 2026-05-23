/**
 * @file task_handler.cpp
 * @brief Xử lý tin nhắn WebSocket nhận từ Dashboard Web.
 *        Phân tích JSON và thực hiện hành động tương ứng:
 *        - page="device": điều khiển LED/NeoPixel/Quạt (chỉ khi MANUAL mode)
 *        - page="auto": chuyển đổi AUTO ↔ MANUAL
 *        - page="setting": lưu cấu hình WiFi/CoreIoT và yêu cầu restart
 */

#include "task_handler.h"
#include "global.h"
#include <ArduinoJson.h>

extern SystemState* appState;
extern void Save_info_File(String wifi_ssid, String wifi_pass, String CORE_IOT_TOKEN, String CORE_IOT_SERVER, String CORE_IOT_PORT);

#define FAN_PIN 6 // Chân GPIO điều khiển quạt tản nhiệt

// Xử lý message JSON nhận từ WebSocket client
void handleWebSocketMessage(String msg) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        Serial.printf("JSON Parse Error: %s\n", error.c_str());
        return;
    }

    String page = doc["page"].as<String>(); // Xác định loại lệnh dựa trên trường "page"

    if (page == "device") {
        // Lệnh điều khiển thiết bị: {"page":"device", "value":{"device":"fan","status":true}}
        String dev = doc["value"]["device"].as<String>();
        bool status = doc["value"]["status"].as<bool>();

        if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Chỉ cho phép điều khiển khi đang ở MANUAL mode
            if (appState->isAutoMode == false) { 
                if (dev == "fan") {
                    appState->fanState = status;
                    pinMode(FAN_PIN, OUTPUT);
                    digitalWrite(FAN_PIN, status ? HIGH : LOW); // Điều khiển quạt trực tiếp qua GPIO
                    Serial.println(status ? "Web: Bật Quạt" : "Web: Tắt Quạt");
                } 
                else if (dev == "led") {
                    appState->ledState = status; // LED task sẽ đọc giá trị này trong vòng lặp tiếp theo
                    Serial.println(status ? "Web: Bật LED" : "Web: Tắt LED");
                }
                else if (dev == "neo") {
                    appState->neoState = status; // NeoPixel task sẽ đọc giá trị này
                    Serial.println(status ? "Web: Bật NEO" : "Web: Tắt NEO");
                }
            } else {
                Serial.println("Hệ thống đang ở chế độ AUTO!"); // Từ chối lệnh khi AUTO
            }
            xSemaphoreGive(appState->dataMutex);
        }
    } 
    else if (page == "auto") {
        // Chuyển đổi chế độ: {"page":"auto", "value":{"mode":true}}
        if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            appState->isAutoMode = doc["value"]["mode"].as<bool>();
            Serial.println(appState->isAutoMode ? "Chuyen sang AUTO" : "Chuyen sang MANUAL");
            xSemaphoreGive(appState->dataMutex);
        }
    } 
    else if (page == "setting") {
        // Lưu cấu hình mạng: {"page":"setting", "value":{"ssid":"...","password":"...","token":"...","server":"...","port":"..."}}
        String ssid = doc["value"]["ssid"].as<String>();
        String pass = doc["value"]["password"].as<String>();
        String token = doc["value"]["token"].as<String>();
        String server = doc["value"]["server"].as<String>();
        String port = doc["value"]["port"].as<String>();

        if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            appState->wifiSSID = ssid; appState->wifiPass = pass;
            appState->coreIotToken = token; appState->coreIotServer = server;
            appState->coreIotPort = port;
            
            appState->needsRestart = true; // Yêu cầu restart để áp dụng cấu hình mới
            xSemaphoreGive(appState->dataMutex);
        }
        Save_info_File(ssid, pass, token, server, port); // Ghi vào LittleFS
    }
}