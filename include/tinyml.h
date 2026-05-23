#ifndef __TINY_ML__
#define __TINY_ML__

#include <Arduino.h>

#include "my_dht_anomaly_model.h" // Mô hình AI đã train
#include "global.h"

#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

void setupTinyML();                     // Khởi tạo model, interpreter và cấp phát tensor arena
void tiny_ml_task(void *pvParameters);  // Nhận semaphore → chuẩn hóa → inference → cập nhật kết quả

#endif