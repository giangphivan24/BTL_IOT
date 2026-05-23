/**
 * @file main.cpp
 * @brief Điểm khởi đầu của hệ thống giám sát môi trường IoT trên ESP32-S3.
 *        - Khởi tạo phần cứng (I2C, LittleFS, WiFi)
 *        - Tạo cấu trúc dữ liệu dùng chung SystemState
 *        - Tạo các Mutex/Semaphore cho cơ chế đồng bộ FreeRTOS
 *        - Khởi chạy tất cả FreeRTOS tasks song song
 *        - Vòng loop() kiểm tra cờ restart và duy trì OTA
 */

#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include "global.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "lcd_display.h"
#include "tinyml.h"

#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"

SystemState* appState = nullptr;
extern void webserver_push_task(void *pvParameters);

void setup() {
    Serial.begin(115200);
    delay(3000); 
    Serial.println("\n--- Thiết bị đang khởi động ---");

    Wire.begin(11, 12); // Khởi tạo I2C: SDA = GPIO 11, SCL = GPIO 12 (cho DHT20 và LCD)

    // Khởi tạo hệ thống file Flash để đọc cấu hình và phục vụ Web
    if (!LittleFS.begin(true)) {
        Serial.println("Lỗi khởi động LittleFS!");
    } else {
        Serial.println("LittleFS đã sẵn sàng.");
    }
    
    // Cấp phát và khởi tạo giá trị mặc định cho cấu trúc dữ liệu dùng chung
    appState = new SystemState();
    appState->temperature = 0.0; appState->humidity = 0.0;
    appState->displayState = 0; appState->tinyML_Anomaly = false;
    appState->isAutoMode = true; appState->ledState = false; appState->neoState = false; appState->fanState = false;
    appState->wifiSSID = ""; appState->wifiPass = "";         
    appState->coreIotToken = ""; appState->coreIotServer = "app.coreiot.io"; 
    appState->coreIotPort = "1883"; appState->hasInternet = false;
    appState->needsRestart = false; 

    // Tạo Mutex và Binary Semaphore cho cơ chế đồng bộ giữa các task
    appState->dataMutex = xSemaphoreCreateMutex();
    appState->sensorReadySync = xSemaphoreCreateBinary();
    appState->humiReadySync = xSemaphoreCreateBinary();
    appState->lcdReadySync = xSemaphoreCreateBinary();
    appState->mlReadySync = xSemaphoreCreateBinary();
    appState->internetReadySync = xSemaphoreCreateBinary();

    Load_info_File(); // Đọc cấu hình WiFi/CoreIoT đã lưu từ LittleFS

    Serial.printf("SSID đã lưu: [%s]\n", appState->wifiSSID.c_str());
    Serial.printf("Token đã lưu: [%s]\n", appState->coreIotToken.c_str());

    WiFi.mode(WIFI_AP_STA); // Chạy đồng thời AP (cấu hình) + STA (Internet)
    startAP();     // Bật Access Point nội bộ để người dùng truy cập Dashboard
    connnectWSV(); // Khởi tạo AsyncWebServer + WebSocket + OTA

    // Tạo các FreeRTOS tasks
    // Priority cao hơn = ưu tiên cao hơn. Sensor và WiFi ưu tiên cao nhất (3).
    xTaskCreate(temp_humi_monitor, "Task DHT", 4096, (void*)appState, 3, NULL);
    xTaskCreate(tiny_ml_task, "Tiny ML Task", 8192, (void*)appState, 2, NULL);   
    xTaskCreate(lcd_display_task, "Task LCD", 4096, (void*)appState, 2, NULL);
    
    xTaskCreate(wifi_task, "WiFi Task", 4096, (void*)appState, 3, NULL);
    xTaskCreate(coreiot_task, "CoreIOT Task", 4096, (void*)appState, 2, NULL);
    
    xTaskCreate(led_blinky, "Task LED", 2048, (void*)appState, 1, NULL);
    xTaskCreate(neo_blinky, "Task NEO", 2048, (void*)appState, 1, NULL);
    xTaskCreate(webserver_push_task, "Web Push Task", 4096, (void*)appState, 1, NULL);
    xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 2048, NULL, 1, NULL);
}

// Vòng lặp chính: chỉ kiểm tra cờ restart và duy trì OTA update
void loop() {
    bool restart = false;
    if (xSemaphoreTake(appState->dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        restart = appState->needsRestart;
        xSemaphoreGive(appState->dataMutex);
    }

    if (restart) {
        Serial.println("Chuẩn bị khởi động lại...");
        vTaskDelay(pdMS_TO_TICKS(1000)); 
        ESP.restart();
    }

    ElegantOTA.loop(); // Xử lý OTA firmware update nếu có yêu cầu
    vTaskDelay(pdMS_TO_TICKS(20)); 
}