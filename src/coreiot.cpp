#include "coreiot.h"
#include "global.h"

// ----------- CONFIGURE THESE! -----------
const char* coreIOT_Server = "10.235.76.226";
const char* coreIOT_Token  = "g7drm1amhd3dchr379xu";
const int mqttPort = 1883;
// ----------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");

        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);

        // CoreIOT / ThingsBoard dùng token làm username
        if (client.connect(clientId.c_str(), coreIOT_Token, NULL)) {
            Serial.println("connected to CoreIOT Server!");

            client.subscribe("v1/devices/me/rpc/request/+");
            Serial.println("Subscribed to RPC topic");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.println("]");

    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    Serial.print("Payload: ");
    Serial.println(message);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    const char* method = doc["method"];

    if (strcmp(method, "setPump") == 0) {
        bool state = doc["params"];

        autoMode = false;
        pumpOn = state;
        digitalWrite(PUMP_PIN, pumpOn ? HIGH : LOW);

        Serial.print("Pump RPC: ");
        Serial.println(pumpOn ? "ON" : "OFF");
    }
    else if (strcmp(method, "setAutoMode") == 0) {
        autoMode = doc["params"];

        Serial.print("Auto mode RPC: ");
        Serial.println(autoMode ? "ON" : "OFF");
    }
    else {
        Serial.print("Unknown method: ");
        Serial.println(method);
    }
}

void setup_coreiot() {
    while (1) {
        if (xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY)) {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }

    Serial.println("WiFi connected for CoreIOT!");

    client.setServer(coreIOT_Server, mqttPort);
    client.setCallback(callback);
}

void coreiot_task(void *pvParameters) {
    setup_coreiot();

    while (1) {
        if (!client.connected()) {
            reconnect();
        }

        client.loop();

        String payload = "{";
        payload += "\"soil_moisture\":" + String(soilPercent) + ",";
        payload += "\"pump\":\"" + String(pumpOn ? "ON" : "OFF") + "\",";
        payload += "\"auto_mode\":" + String(autoMode ? "true" : "false") + ",";
        payload += "\"soil_status\":\"" + soilStatus + "\"";
        payload += "}";

        client.publish("v1/devices/me/telemetry", payload.c_str());

        Serial.println("Published payload: " + payload);

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
