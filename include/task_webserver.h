#ifndef __TASK_WEBSERVER_H__
#define __TASK_WEBSERVER_H__

#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <ElegantOTA.h>
#include <task_handler.h>

extern AsyncWebServer server; // WebServer bất đồng bộ trên port 80
extern AsyncWebSocket ws;     // WebSocket endpoint tại /ws

void Webserver_stop();                      // Đóng WebSocket và dừng server
void Webserver_reconnect();                 // Khởi động lại server nếu đã bị dừng
void Webserver_sendata(String data);        // Gửi dữ liệu tới tất cả WebSocket client (legacy)
void connnectWSV();                         // Khởi tạo route, WebSocket handler, OTA và bật server

#endif