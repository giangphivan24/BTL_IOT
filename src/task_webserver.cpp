/**
 * @file task_webserver.cpp
 * @brief Module WebServer bất đồng bộ trên ESP32.
 *        - Phục vụ Dashboard Web (HTML/CSS/JS) từ LittleFS
 *        - WebSocket endpoint /ws: push dữ liệu realtime + nhận lệnh điều khiển
 *        - Route /save: lưu cấu hình WiFi/CoreIoT vào LittleFS
 *        - Tích hợp ElegantOTA cho firmware update qua Web
 */

#include "task_webserver.h"
#include "global.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "task_handler.h"
#include "task_check_info.h"

AsyncWebServer server(80);   // WebServer bất đồng bộ trên port 80
AsyncWebSocket ws("/ws");    // WebSocket endpoint tại đường dẫn /ws

bool webserver_isrunning = false; // Cờ theo dõi trạng thái server đang chạy hay không

void webserver_push_task(void *pvParameters) {
  SystemState *state = (SystemState *)pvParameters;
  while (1) {
    // Chỉ gửi khi server đang chạy VÀ có ít nhất 1 client kết nối
    if (webserver_isrunning && ws.count() > 0 && state != nullptr) {
      float temp = 0.0f, humi = 0.0f;
      int sysStatus = 0;
      bool aiStatus = false, autoMode = true, led = false, neo = false,
           fan = false;

      // Đọc toàn bộ trạng thái hệ thống an toàn qua Mutex
      if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        temp = state->temperature;
        humi = state->humidity;
        sysStatus = state->displayState;
        aiStatus = state->tinyML_Anomaly;
        autoMode = state->isAutoMode;
        led = state->ledState;
        neo = state->neoState;
        fan = state->fanState;
        xSemaphoreGive(state->dataMutex);
      }

      // Đóng gói JSON gửi tới Dashboard Web
      StaticJsonDocument<256> doc;
      doc["type"] = "update";
      doc["temp"] = temp;
      doc["humi"] = humi;
      doc["sys"] = sysStatus;   // 0 = Normal, 1 = Warning, 2 = Critical
      doc["ai"] = aiStatus;     // true = AI phát hiện bất thường
      doc["auto"] = autoMode;   // true = đang ở chế độ AUTO
      doc["led"] = led;
      doc["neo"] = neo;
      doc["fan"] = fan;

      String payload;
      serializeJson(doc, payload);
      ws.textAll(payload); // Broadcast tới tất cả client đang kết nối
    }
    vTaskDelay(pdMS_TO_TICKS(2000)); // Chu kỳ push: 2 giây
  }
}

// Callback xử lý sự kiện WebSocket: kết nối, ngắt kết nối, nhận dữ liệu
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT)
    Serial.printf("WebSocket client #%u connected\n", client->id());
  else if (type == WS_EVT_DISCONNECT)
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
  else if (type == WS_EVT_DATA) {
    // Nhận dữ liệu text từ client → chuyển sang handler xử lý lệnh
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->opcode == WS_TEXT) {
      String message = "";
      for (size_t i = 0; i < len; i++)
        message += (char)data[i];
      handleWebSocketMessage(message); 
    }
  }
}

// Khởi tạo toàn bộ WebServer: đăng ký route, WebSocket handler và OTA
void connnectWSV() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Route phục vụ giao diện Dashboard từ LittleFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/script.js", "application/javascript");
  });
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/styles.css", "text/css");
  });

  // Route POST /save: nhận cấu hình WiFi/CoreIoT từ form cài đặt trên Dashboard
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true))
      appState->wifiSSID = request->getParam("ssid", true)->value();
    if (request->hasParam("pass", true))
      appState->wifiPass = request->getParam("pass", true)->value();
    if (request->hasParam("token", true))
      appState->coreIotToken = request->getParam("token", true)->value();
    if (request->hasParam("server", true))
      appState->coreIotServer = request->getParam("server", true)->value();
    if (request->hasParam("port", true))
      appState->coreIotPort = request->getParam("port", true)->value();

    // Ghi cấu hình mới vào bộ nhớ Flash (LittleFS)
    Save_info_File(appState->wifiSSID, appState->wifiPass,
                   appState->coreIotToken, appState->coreIotServer,
                   appState->coreIotPort);

    request->send(200, "text/plain", "OK");

    Serial.println(
        ">>> Đã nhận lệnh lưu. Hệ thống sẽ khởi động lại từ loop()...");
    appState->needsRestart = true; // Đặt cờ restart để loop() thực hiện ESP.restart()
  });

  server.begin();
  ElegantOTA.begin(&server); // Kích hoạt OTA update tại đường dẫn /update
  webserver_isrunning = true;
  Serial.println("AsyncWebServer đã khởi động thành công!");
}

// Dừng WebServer: đóng tất cả WebSocket và tắt server
void Webserver_stop() {
  ws.closeAll();
  server.end();
  webserver_isrunning = false;
}

// Khởi động lại server nếu đã bị dừng (dùng khi WiFi reconnect)
void Webserver_reconnect() {
  if (!webserver_isrunning) {
    connnectWSV();
  }
  ElegantOTA.loop();
}