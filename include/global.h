/**
 * @file global.h
 * @brief Định nghĩa cấu trúc dữ liệu dùng chung (SystemState) cho toàn bộ hệ thống.
 *        Mọi task FreeRTOS đều truy cập struct này thông qua con trỏ appState.
 *        Dữ liệu được bảo vệ bởi Mutex và đồng bộ bằng Binary Semaphore.
 */

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct SystemState {
    // Dữ liệu cảm biến
    float temperature;      // Nhiệt độ trung bình sau lọc Moving Average (°C)
    float humidity;         // Độ ẩm trung bình sau lọc Moving Average (%)
    
    // Trạng thái hệ thống
    int displayState;       // Mức cảnh báo: 0 = Normal, 1 = Warning, 2 = Critical
    bool tinyML_Anomaly;    // Kết quả suy luận AI: true = phát hiện bất thường

    // Chế độ điều khiển thiết bị
    bool isAutoMode;        // true = AUTO (hệ thống tự điều khiển), false = MANUAL (người dùng điều khiển)
    bool ledState;          // Trạng thái LED đơn (dùng trong MANUAL mode)
    bool neoState;          // Trạng thái NeoPixel RGB (dùng trong MANUAL mode)
    bool fanState;          // Trạng thái quạt tản nhiệt (dùng trong MANUAL mode)

    // Cấu hình mạng 
    String wifiSSID;        // Tên WiFi để kết nối Internet (STA mode)
    String wifiPass;        // Mật khẩu WiFi
    String coreIotToken;    // Token xác thực thiết bị trên CoreIoT
    String coreIotServer;   // Địa chỉ server CoreIoT (mặc định: app.coreiot.io)
    String coreIotPort;     // Cổng MQTT (mặc định: 1883)
    bool hasInternet;       // Cờ báo đã kết nối WiFi STA thành công
    bool needsRestart;      // Cờ yêu cầu khởi động lại (sau khi lưu cấu hình mới)

    // Cơ chế đồng bộ FreeRTOS
    SemaphoreHandle_t dataMutex;        // Mutex bảo vệ toàn bộ struct khi đọc/ghi từ nhiều task
    SemaphoreHandle_t sensorReadySync;  // Semaphore báo dữ liệu cảm biến mới cho LED task
    SemaphoreHandle_t humiReadySync;    // Semaphore báo dữ liệu mới cho NeoPixel task
    SemaphoreHandle_t lcdReadySync;     // Semaphore báo dữ liệu mới cho LCD task
    SemaphoreHandle_t mlReadySync;      // Semaphore báo dữ liệu mới cho TinyML task
    SemaphoreHandle_t internetReadySync;// Semaphore báo WiFi STA đã kết nối cho CoreIoT task
};

extern SystemState* appState; // Con trỏ toàn cục

#endif