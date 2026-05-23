#include "task_handler.h"
#include "global.h"
#include "lcd_display.h"

extern LiquidCrystal_I2C lcd;

void handleWebSocketMessage(String message)
{
    Serial.println("WS Received: " + message);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.println("JSON parse failed");
        return;
    }

    String device = doc["device"] | "";
    String action = doc["action"] | "";
    String value  = doc["value"]  | "";

    // ===== Điều khiển máy bơm =====
    if (device == "pump")
    {
        if (action == "ON")
        {
            digitalWrite(PUMP_PIN, HIGH);
            Serial.println("Pump ON Manual");
        }
        else if (action == "OFF")
        {
            digitalWrite(PUMP_PIN, LOW);
            Serial.println("Pump OFF Manual");
        }
    }

    // ===== Điều khiển LCD =====
    if (device == "lcd")
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(value.substring(0, 16));

        lcd.setCursor(0, 1);
        lcd.print("From Webserver");

        Serial.println("LCD Updated: " + value);
    }
}
