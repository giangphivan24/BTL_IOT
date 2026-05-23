#ifndef __TASK_CHECK_INFO_H__
#define __TASK_CHECK_INFO_H__

#include <ArduinoJson.h>
#include "LittleFS.h"
#include "global.h"
#include "task_wifi.h"

bool check_info_File(bool check);  // Kiểm tra xem đã có cấu hình WiFi hay chưa
void Load_info_File();             // Đọc cấu hình từ /info.dat vào SystemState
void Delete_info_File();           // Xóa /info.dat và khởi động lại ESP32
void Save_info_File(String WIFI_SSID, String WIFI_PASS, String CORE_IOT_TOKEN, String CORE_IOT_SERVER, String CORE_IOT_PORT); // Ghi cấu hình mới vào LittleFS

#endif