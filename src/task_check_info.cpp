/**
 * @file task_check_info.cpp
 * @brief Module quản lý cấu hình hệ thống trên bộ nhớ Flash (LittleFS).
 *        File /info.dat lưu dạng JSON chứa: WIFI_SSID, WIFI_PASS, CORE_IOT_TOKEN, CORE_IOT_SERVER, CORE_IOT_PORT.
 *        Cấu hình được đọc khi khởi động và ghi lại khi người dùng thay đổi từ Dashboard.
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "global.h"

extern SystemState* appState;

extern void startAP(); 

// Đọc cấu hình từ file /info.dat trên LittleFS vào SystemState
void Load_info_File()
{
    File file = LittleFS.open("/info.dat", "r");
    if (!file) {
        return; // File chưa tồn tại (lần đầu chạy) → giữ giá trị mặc định
    }
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    
    if (error) {
        Serial.println("❌ Lỗi giải mã file info.dat!");
    } else {
        // Ghi vào SystemState qua Mutex
        if (xSemaphoreTake(appState->dataMutex, portMAX_DELAY) == pdTRUE) {
            appState->wifiSSID = doc["WIFI_SSID"] | "";
            appState->wifiPass = doc["WIFI_PASS"] | "";
            appState->coreIotToken = doc["CORE_IOT_TOKEN"] | "";
            appState->coreIotServer = doc["CORE_IOT_SERVER"] | "10.235.76.226";
            appState->coreIotPort = doc["CORE_IOT_PORT"] | "1883";
            xSemaphoreGive(appState->dataMutex);
        }
        Serial.println("Đã nạp cấu hình mạng thành công!");
    }
    file.close();
}

// Xóa file cấu hình và khởi động lại (factory reset)
void Delete_info_File()
{
    if (LittleFS.exists("/info.dat")) {
        LittleFS.remove("/info.dat");
        Serial.println("Đã xóa cấu hình mạng!");
    }
    ESP.restart();
}

// Ghi cấu hình mới vào file /info.dat trên LittleFS 
void Save_info_File(String wifi_ssid, String wifi_pass, String core_iot_token, String core_iot_server, String core_iot_port)
{
    DynamicJsonDocument doc(1024);
    doc["WIFI_SSID"] = wifi_ssid;
    doc["WIFI_PASS"] = wifi_pass;
    doc["CORE_IOT_TOKEN"] = core_iot_token;
    doc["CORE_IOT_SERVER"] = core_iot_server;
    doc["CORE_IOT_PORT"] = core_iot_port;

    File configFile = LittleFS.open("/info.dat", "w");
    if (configFile) {
        serializeJson(doc, configFile); // Serialize JSON trực tiếp vào file
        configFile.close();
        Serial.println("Đã lưu cấu hình mới!");
    } else {
        Serial.println("❌ Lỗi: Không thể lưu cấu hình.");
    }
}

// Kiểm tra xem đã có cấu hình WiFi hay chưa (dùng khi khởi động)
bool check_info_File(bool check)
{
    if (!check) {
        // Lần gọi đầu tiên: khởi tạo LittleFS và đọc file cấu hình
        if (!LittleFS.begin(true)) {
            Serial.println("❌ Lỗi khởi động LittleFS!");
            return false;
        }
        Load_info_File();
    }
  
    String ssid = "";
    if (xSemaphoreTake(appState->dataMutex, portMAX_DELAY) == pdTRUE) {
        ssid = appState->wifiSSID;
        xSemaphoreGive(appState->dataMutex);
    }

    if (ssid.isEmpty()) {
        if (!check) {
            startAP(); // Chưa có SSID → bật AP để người dùng cấu hình lần đầu
        }
        return false;
    }
    return true; 
}