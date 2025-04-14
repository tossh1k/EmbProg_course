#include <Arduino.h>
#include <SFE_BMP180.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define LED_PIN 2
int state = HIGH;

SFE_BMP180 bmp180;
double temperature = 0, pressure = 0;

SemaphoreHandle_t SemaphoreBin = NULL; 
SemaphoreHandle_t mutex = NULL; 

TaskHandle_t wait_press_read;
TaskHandle_t wait_temp_read;

void LedTask(void* Arg) {
  pinMode(LED_PIN, OUTPUT);

  for (;;) {
      state = !state;
      digitalWrite(LED_PIN, state);
      vTaskDelay(pdMS_TO_TICKS(1000));
  }

  vTaskDelete(NULL);
}


void read_temp(void* Arg) {
  for (;;) {
      xSemaphoreTake(mutex, portMAX_DELAY);
      char status = bmp180.startTemperature();
      if (status > 0) {
        delay(status);
        status = bmp180.getTemperature(temperature);
      }
      xSemaphoreGive(mutex);
      xTaskNotifyGive(*((TaskHandle_t*)Arg));
      vTaskDelay(pdMS_TO_TICKS(5000));
    }

    vTaskDelete(NULL);
}

void get_temp(void* Arg) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    xSemaphoreTake(SemaphoreBin, portMAX_DELAY);
    Serial.print("Temprature: ");
    Serial.println(temperature);
    xSemaphoreGive(SemaphoreBin);
  }

  vTaskDelete(NULL);
}

void read_press(void* Arg) {
  for (;;) {
      xSemaphoreTake(mutex, portMAX_DELAY);
      char status = bmp180.startTemperature();
      
      if (status > 0) {
        delay(status);
        status = bmp180.getTemperature(temperature);
        
        if (status > 0) {
          status = bmp180.startPressure(2);

          if (status > 0) {
            delay(status);
            status = bmp180.getPressure(pressure, temperature);
          }
        }
      }
      pressure = pressure*76000/101325;
      xSemaphoreGive(mutex);
      xTaskNotifyGive(*((TaskHandle_t*)Arg));
      vTaskDelay(pdMS_TO_TICKS(5000));
    }

    vTaskDelete(NULL);
}

void get_press(void* Arg) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    xSemaphoreTake(SemaphoreBin, portMAX_DELAY);
    Serial.print("Pressure: ");
    Serial.println(pressure);
    xSemaphoreGive(SemaphoreBin);
  }

  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Grigoryev NI");
  bmp180.begin();

  SemaphoreBin = xSemaphoreCreateBinary();
  xSemaphoreGive(SemaphoreBin);
  mutex = xSemaphoreCreateMutex();

  xTaskCreate(LedTask, "ledstate", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);

  xTaskCreate(read_temp, "MeasureTemp", 2048, &wait_temp_read, tskIDLE_PRIORITY + 1, NULL);
  xTaskCreate(get_temp, "PrintTemp", 2048, NULL, tskIDLE_PRIORITY + 1, &wait_temp_read);

  xTaskCreate(read_press, "MeasurePress", 2048, &wait_press_read, tskIDLE_PRIORITY + 1, NULL);
  xTaskCreate(get_press, "PrintPress", 2048, NULL, tskIDLE_PRIORITY + 1, &wait_press_read);
}

void loop() {

}

