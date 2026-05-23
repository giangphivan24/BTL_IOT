#include "led_blinky.h"
#include "global.h"

void led_blinky(void *pvParameters)
{
  pinMode(LED_GPIO, OUTPUT);

  SensorData data;
  int delayTime = 1000;

  while (1)
  {
    if (xQueueReceive(queueLED, &data, portMAX_DELAY))
    {
      if (data.temp < 25)
        delayTime = 1000;
      else if (data.temp < 35)
        delayTime = 500;
      else
        delayTime = 100;
    }

    digitalWrite(LED_GPIO, HIGH);
    vTaskDelay(pdMS_TO_TICKS(delayTime));

    digitalWrite(LED_GPIO, LOW);
    vTaskDelay(pdMS_TO_TICKS(delayTime));
  }
}
