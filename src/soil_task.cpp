#include <Arduino.h>
#include "global.h"

void soil_task(void *pvParameters) {
    while (1) {
        int soilRaw = analogRead(SOIL_PIN);
        soilPercent = map(soilRaw, 0, 4095, 100, 0);
        soilPercent = constrain(soilPercent, 0, 100);

        if (soilPercent < 30) {
            soilStatus = "DRY";
        } else if (soilPercent < 60) {
            soilStatus = "NORMAL";
        } else {
            soilStatus = "WET";
        }

        if (autoMode) {
            if (soilPercent < 30) {
                pumpOn = true;
            } else {
                pumpOn = false;
            }

            digitalWrite(PUMP_PIN, pumpOn ? HIGH : LOW);
        }

        Serial.print("Soil: ");
        Serial.print(soilPercent);
        Serial.print("% | Pump: ");
        Serial.print(pumpOn ? "ON" : "OFF");
        Serial.print(" | Status: ");
        Serial.println(soilStatus);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}