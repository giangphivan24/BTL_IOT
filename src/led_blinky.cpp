/**
 * @file led_blinky.cpp
 * @brief Task điều khiển LED đơn onboard (GPIO 48).
 *        Tốc độ nhấp nháy phản ánh mức cảnh báo: Normal (5s) → Warning (2s) → Critical (0.2s).
 *        Hỗ trợ 2 chế độ: AUTO (nháy theo trạng thái) và MANUAL (bật/tắt từ Web).
 */

#include "led_blinky.h"

void led_blinky(void *pvParameters) {
  SystemState *state = (SystemState *)pvParameters;
  pinMode(LED_GPIO, OUTPUT);

  bool localLedState = false;
  bool isAuto = true;
  int localDispState = 0;
  bool blinkState = false;

  TickType_t blinkDelay = pdMS_TO_TICKS(5000); // Tốc độ nháy mặc định: 5 giây (Normal)
  TickType_t lastToggleTime = xTaskGetTickCount();

  while (1) {
    // Đọc trạng thái hệ thống an toàn qua Mutex
    if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      isAuto = state->isAutoMode;
      localLedState = state->ledState;
      localDispState = state->displayState;
      xSemaphoreGive(state->dataMutex);
    }

    // Điều chỉnh tốc độ nháy theo 3 mức cảnh báo
    if (localDispState == 2) {
        blinkDelay = pdMS_TO_TICKS(200);  // Critical: nháy rất nhanh (0.2s)
    } else if (localDispState == 1) {
        blinkDelay = pdMS_TO_TICKS(2000); // Warning: nháy trung bình (2s)
    } else {
        blinkDelay = pdMS_TO_TICKS(5000); // Normal: nháy chậm (5s)
    }

    // MANUAL mode: nếu người dùng tắt LED từ web → tắt và bỏ qua logic nháy
    if (!isAuto) {
      if (!localLedState) {
          digitalWrite(LED_GPIO, LOW);
          blinkState = false;
          vTaskDelay(pdMS_TO_TICKS(100));
          lastToggleTime = xTaskGetTickCount();
          continue;
      }
    }

    // AUTO hoặc MANUAL (khi đã bật): toggle LED theo chu kỳ blinkDelay
    TickType_t now = xTaskGetTickCount();
    if (now - lastToggleTime >= blinkDelay) {
      blinkState = !blinkState;
      digitalWrite(LED_GPIO, blinkState ? HIGH : LOW);
      lastToggleTime = now;
    }

    vTaskDelay(pdMS_TO_TICKS(50)); // Polling ngắn để phản hồi nhanh khi đổi trạng thái
  }
}