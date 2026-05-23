#include "temp_humi_monitor.h"
#include "global.h"
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);


void temp_humi_monitor(void *pvParameters){

    Wire.begin(11, 12);
    Serial.begin(115200);
    dht20.begin();

    while (1){
        /* code */
        
        dht20.read();
        // Reading temperature in Celsius
        float temperature = dht20.getTemperature();
        // Reading humidity
        float humidity = dht20.getHumidity();

        

        // Check if any reads failed and exit early
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity =  -1;
            //return;
        }

        //Update global variables for temperature and humidity
        SensorData data;
        data.temp = temperature;
        data.hum = humidity;
        xQueueSend(queueLED, &data, portMAX_DELAY);
        xQueueSend(queueNeo, &data, portMAX_DELAY);
        xQueueSend(dataQueue, &data, portMAX_DELAY);
        xQueueSend(queueCoreIoT, &data, portMAX_DELAY);
        xQueueSend(queueML, &data, portMAX_DELAY);
        // Print the results
        
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");
        
        vTaskDelay(5000);
    }
    
}

void lcd_task(void *pvParameters) {
    SensorData data;
    lcd.begin();
    lcd.backlight();
    while(1) {
        if (xQueueReceive(dataQueue, &data, portMAX_DELAY)) {
            lcd.clear();
            if (data.temp < 35 && data.hum < 70) {
                lcd.setCursor(0,0);
                lcd.print("Normal");
            }
            else if (data.temp < 40) {
                lcd.setCursor(0,0);
                lcd.print("Warning");
            }
            else {
                lcd.setCursor(0,0);
                lcd.print("CRITICAL");
            }
            
            lcd.setCursor(0,1);
            lcd.print("T: ");
            lcd.print(data.temp);
            lcd.print(" H: ");
            lcd.print(data.hum);
        }
    }
}
