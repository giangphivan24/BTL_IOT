#ifndef __TASK_WIFI_H__
#define __TASK_WIFI_H__

#include <WiFi.h>
#include <task_check_info.h>
#include <task_webserver.h>

extern bool Wifi_reconnect();              // Thử kết nối lại WiFi STA (legacy)
extern void startAP();                     // Bật Access Point nội bộ để người dùng cấu hình
void wifi_task(void *pvParameters);        // Tự động kết nối WiFi STA và retry khi mất mạng

#endif