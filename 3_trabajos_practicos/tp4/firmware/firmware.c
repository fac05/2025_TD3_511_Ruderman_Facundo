#include <stdio.h>
#include "pico/stdlib.h"
#include "lcd.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bmp280.h"
#include "semphr.h"

#define PIN_SDA 4
#define PIN_SCL 5

typedef struct {
    float temperatura;
    int32_t presion;
} SensorData;
struct bmp280_calib_param params;

QueueHandle_t xQueueSensorData;
SemaphoreHandle_t xSemaphoreI2C;

void sensorTask(void *pvParameters) {
    SensorData data;
    int32_t raw_temperature, raw_pressure;
    while (1) {
        if (xSemaphoreTake(xSemaphoreI2C, portMAX_DELAY) == pdTRUE) {
            bmp280_get_calib_params(&params);

            // Obtiene valores sin compensar
            
            bmp280_read_raw(&raw_temperature, &raw_pressure);

            // Obtiene los valores compensados de temperatura y presión
            data.temperatura = bmp280_convert_temp(raw_temperature, &params);
            data.presion = bmp280_convert_pressure(raw_pressure, raw_temperature, &params);
            printf("Valor de presion: %d\n", data.presion);
            printf("Valor de temperatura: %.2f\n", data.temperatura);
            data.presion /= 1000.0;  // Convertir a kPa
            xSemaphoreGive(xSemaphoreI2C);
        }

        xQueueSend(xQueueSensorData, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));  // Cada 1 segundo
    }
}

void lcdTask(void *pvParameters) {
    SensorData data;
    char buffer[20];

    while (1) {
        //lcd_set_cursor(0, 0);
        //lcd_string("HOLA MUNDO");
        //printf("¿Llega a la tarea de print?");
        if (xQueueReceive(xQueueSensorData, &data, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(xSemaphoreI2C, portMAX_DELAY) == pdTRUE) {
                lcd_clear();
                snprintf(buffer, sizeof(buffer), "Temp: %.2f C", data.temperatura);
                lcd_set_cursor(0, 0);
                lcd_string(buffer);

                snprintf(buffer, sizeof(buffer), "Pres: %.d kPa", data.presion);
                lcd_set_cursor(1, 0);
                lcd_string(buffer);
                xSemaphoreGive(xSemaphoreI2C);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


int main() {
    stdio_init_all();
    i2c_init(i2c0, 100000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);  // SDA
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);  // SCL
    //i2c_set_slave_mode(i2c0, false, 0);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    bmp280_init(i2c0);   // Inicializa el sensor
    lcd_init(i2c0, 0x27  );      // Inicializa el LCD
    //lcd_set_cursor(0, 0);
    //lcd_string("HOLA MUNDO");
    xQueueSensorData = xQueueCreate(10, sizeof(SensorData));
    xSemaphoreI2C = xSemaphoreCreateMutex();

    xTaskCreate(sensorTask, "Sensor", 4*configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(lcdTask, "LCD", 4*configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {}
    return 0;
}
