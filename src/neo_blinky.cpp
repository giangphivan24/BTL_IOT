#include "neo_blinky.h"
#include "global.h"

void neo_blinky(void *pvParameters)
{
    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show();

    SensorData data;

    while (1)
    {
        if (xQueueReceive(queueNeo, &data, portMAX_DELAY))
        {
            if (data.hum < 40)
            {
                strip.setPixelColor(0, strip.Color(0, 0, 255)); // Blue
            }
            else if (data.hum < 70)
            {
                strip.setPixelColor(0, strip.Color(0, 255, 0)); // Green
            }
            else
            {
                strip.setPixelColor(0, strip.Color(255, 0, 0)); // Red
            }

            strip.show();
        }
    }
}
