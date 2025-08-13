#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "lcd.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bmp280.h"
#include "semphr.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"


#define PIN_SDA 4
#define PIN_SCL 5
#define SETPOINT_TEMP 20.0f
#define BUTTON_PIN 10
#define PWM_GPIO 15
#define PWM_SLICE_NUM pwm_gpio_to_slice_num(PWM_GPIO)

typedef enum {
    PANTALLA_SENSOR,
    PANTALLA_ERROR
} Pantalla;

typedef struct {
    float temperatura;
    int32_t presion;
} SensorData;
struct bmp280_calib_param params;

QueueHandle_t xQueueSensorData;
SemaphoreHandle_t xSemaphoreI2C;
SemaphoreHandle_t buttonSemaphore;

void button_isr(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(buttonSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
void ledTask(void *pvParameters) {
    SensorData data;
    gpio_set_function(PWM_GPIO, GPIO_FUNC_PWM);  // Necesario para activar PWM en el pin

    uint slice_num = pwm_gpio_to_slice_num(PWM_GPIO);
    pwm_set_wrap(slice_num, 100);  // Resolución de 0 a 100
    pwm_set_enabled(slice_num, true);

    while (1) {
        if (xQueuePeek(xQueueSensorData, &data, portMAX_DELAY) == pdTRUE) {
            float error = fabsf(SETPOINT_TEMP - data.temperatura);
            float intensidad = 0;

            if (error < 3.0f) {  // Limitar el máximo error a 10°C
                intensidad = 100.0f - (error * (100.0f / 3.0f));  // Menos error → más luz
            }

            pwm_set_chan_level(slice_num, PWM_CHAN_B, (uint16_t)intensidad);
        }
        vTaskDelay(pdMS_TO_TICKS(200));  // Buen hábito, evitar while(1) pegado
    }
}


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
    float error;
    uint8_t currentScreen = 0; // Variable local a la tarea

    while (1) {
        // No nos bloqueamos: si el botón fue presionado, alternamos pantalla
        if (xSemaphoreTake(buttonSemaphore, 0) == pdTRUE) {
            currentScreen = 1;
        }else {
            currentScreen = 0;
        }


        if (xQueueReceive(xQueueSensorData, &data, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(xSemaphoreI2C, portMAX_DELAY) == pdTRUE) {
                lcd_clear();

                if (currentScreen == 0) {
                    snprintf(buffer, sizeof(buffer), "Temp: %.2f C", data.temperatura);
                    lcd_set_cursor(0, 0);
                    lcd_string(buffer);

                    snprintf(buffer, sizeof(buffer), "Pres: %.d kPa", data.presion);
                    lcd_set_cursor(1, 0);
                    lcd_string(buffer);
                } else {
                    error = fabsf(SETPOINT_TEMP - data.temperatura);
                    snprintf(buffer, sizeof(buffer), "SP: %.2f C", SETPOINT_TEMP);
                    lcd_set_cursor(0, 0);
                    lcd_string(buffer);

                    snprintf(buffer, sizeof(buffer), "Error: %.2f C", error);
                    lcd_set_cursor(1, 0);
                    lcd_string(buffer);
                }

                xSemaphoreGive(xSemaphoreI2C);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}



int main() {
    stdio_init_all();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_isr);

    gpio_init(PWM_GPIO);

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
    buttonSemaphore = xSemaphoreCreateBinary();

    xTaskCreate(sensorTask, "Sensor", 4*configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(lcdTask, "LCD", 4*configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    //xTaskCreate(buttonTask, "Button", 256, NULL, 1, NULL);
    xTaskCreate(ledTask, "LED", 256, NULL, 1, NULL);    
    vTaskStartScheduler();

    while (1) {}
    return 0;
}
