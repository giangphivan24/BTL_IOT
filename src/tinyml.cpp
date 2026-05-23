#include "tinyml.h"

namespace {
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;   // Con trỏ tới tensor đầu vào (2 features: temp, humi)
    TfLiteTensor *output = nullptr;  // Con trỏ tới tensor đầu ra (1 giá trị: anomaly score)
    constexpr int kTensorArenaSize = 8 * 1024; 
    uint8_t tensor_arena[kTensorArenaSize];
}

// Khởi tạo mô hình AI
void setupTinyML() {
    Serial.println("[TinyML] Dang khoi tao TensorFlow Lite Micro...");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite); // Tải mô hình từ mảng byte đã nhúng trong firmware
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("[TinyML ERROR] Model version mismatch!");
        return;
    }

    static tflite::AllOpsResolver resolver; 
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    // Cấp phát vùng nhớ cho các tensor trong model
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("[TinyML ERROR] AllocateTensors() failed!");
        return;
    }

    // Ánh xạ con trỏ input/output để sử dụng khi inference
    input = interpreter->input(0);
    output = interpreter->output(0);
}

// Chờ semaphore từ sensor → chuẩn hóa dữ liệu → chạy inference → cập nhật kết quả
void tiny_ml_task(void *pvParameters) {
    SystemState *state = (SystemState *)pvParameters;
    setupTinyML();

    // Failsafe: tự hủy task nếu khởi tạo AI lỗi, tránh crash toàn hệ thống
    if (interpreter == nullptr) {
        Serial.println("[TinyML] Setup failed. Task stopping.");
        vTaskDelete(NULL); 
        return;
    }

    while (1) {
        // Chỉ thức dậy khi task cảm biến cấp semaphore (có dữ liệu mới)
        if (xSemaphoreTake(state->mlReadySync, portMAX_DELAY) == pdTRUE) {
            
            float currentTemp = 0.0f;
            float currentHumi = 0.0f;
            bool dataOk = false;

            // Đọc dữ liệu cảm biến an toàn qua Mutex
            if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                currentTemp = state->temperature;
                currentHumi = state->humidity;
                xSemaphoreGive(state->dataMutex);
                dataOk = true;
            }

            if (!dataOk) continue; // Bỏ qua nếu không lấy được khóa Mutex

            // Chuẩn hóa Z-score: dùng mean/std tính từ dataset huấn luyện
            const float TEMP_MEAN = 26.30f; const float TEMP_STD = 5.95f;
            const float HUMI_MEAN = 62.80f; const float HUMI_STD = 17.26f;

            input->data.f[0] = (currentTemp - TEMP_MEAN) / TEMP_STD; // Feature 1: nhiệt độ chuẩn hóa
            input->data.f[1] = (currentHumi - HUMI_MEAN) / HUMI_STD; // Feature 2: độ ẩm chuẩn hóa

            unsigned long startTime = millis(); 
            
            // Chạy inference AI trên ESP32
            if (interpreter->Invoke() == kTfLiteOk) {
                unsigned long inferTime = millis() - startTime;
                
                float anomaly_score = output->data.f[0];
                bool isAnomaly = (anomaly_score >= 0.7f); // Ngưỡng quyết định: >= 0.7 → bất thường

                // Cập nhật kết quả AI vào SystemState để NeoPixel, LCD, WebSocket sử dụng
                if (xSemaphoreTake(state->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    state->tinyML_Anomaly = isAnomaly; 
                    xSemaphoreGive(state->dataMutex);
                }

                Serial.printf("[TinyML] T:%.1f H:%.1f | Score: %.3f | %s | %lu ms\n", 
                              currentTemp, currentHumi, anomaly_score, 
                              isAnomaly ? "WARNING (ANOMALY)" : "NORMAL", inferTime);
            } else {
                Serial.println("[TinyML ERROR] Invoke failed");
            }
        }
    }
}