#ifndef __GLOBAL_H__
#define __GLOBAL_H__
#define PUMP_PIN 4
#define SOIL_PIN 1

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern float glob_temperature;
extern float glob_humidity;

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

extern int soilPercent;
extern bool pumpOn;
extern bool autoMode;
extern String soilStatus;

struct SystemState {
    float temperature;   // Biến lưu giá trị nhiệt độ
    float humidity;      // Biến lưu giá trị độ ẩm 
    
    int displayState;   // Biến trạng thái hiển thị LCD 
                        // 0: Normal, 1: Warning , 2: Critical
                         
    bool tinyML_Anomaly;    // Biến dùng để lưu kết quả suy luận của TinyML (Có phải là Anomaly hay không)

    SemaphoreHandle_t dataMutex;     // Mutex bảo vệ dữ liệu chung, tránh xung đột khi đọc/ghi giữa các Task
    
    // Các Semaphore dùng để báo hiệu (kích hoạt) các Task tương ứng
    SemaphoreHandle_t sensorReadySync; // Báo hiệu khi có dữ liệu nhiệt độ mới
    SemaphoreHandle_t humiReadySync;   // Báo hiệu khi có dữ liệu độ ẩm mới
    SemaphoreHandle_t lcdReadySync;    // Báo hiệu cho LCD để cập nhật màn hình
    SemaphoreHandle_t mlReadySync;     // Báo hiệu cho TinyML bắt đầu chạy AI
};

#endif
