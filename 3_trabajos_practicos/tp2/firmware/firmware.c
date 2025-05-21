#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/**
 * @brief Estructura para pasar los datos del sensor
 */
typedef struct {
    uint16_t raw;
    float voltaje;
    float temperatura;
} sensor_temp;

// Cola para datos del sensor
QueueHandle_t queue_sensor;

/**
 * @brief Tarea que escribe por consola
 */
void task_print(void *params) {
    // Estructura para la cola
    sensor_temp data = {0};

    while(1) {
        // Leo el ultimo valor que haya en la cola
        xQueuePeek(queue_sensor, &data, portMAX_DELAY);
        // Saco los datos por terminal
        printf("Temperatura: %.2f C\n\n", data.temperatura);
        // Pequeño delay entre un dato y el siguiente
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Tarea que lee el ADC
 */
void task_adc(void *params) {
    // Declaro estructura para la cola
    sensor_temp data = {0};

    while(1) {
        // Leo el sensor y preparo los datos para la cola
        data.raw = adc_read();
        // Hago la conversión a 12 bits, según documentación.
        data.voltaje = data.raw * 3.3 / (1 << 12);
        // Uso fórmula de la docu para pasar de V a Temp
        data.temperatura = 27 - (data.voltaje - 0.706) / 0.001721;
        // Escribo el último valor de la cola
        xQueueOverwrite(queue_sensor, &data);
    }
}

/**
 * @brief Programa principal
 */
int main(void) {

    stdio_init_all();
    // Inicializacion del ADC  
    adc_init();
    // Inicia el sensor de temperatura conectado al ADC
    adc_set_temp_sensor_enabled(true);
    // Selecciono el canal donde está conectado el ADC (el sensor de temp es el 4)
    adc_select_input(4);
    
    // Creacion de tareas
    xTaskCreate(task_print, "ImprimoTemp", 2 * configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(task_adc, "ADC", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    // Creo la cola donde guardo la temperatura
    queue_sensor = xQueueCreate(1, sizeof(sensor_temp));
    
    // Arranca el SO
    vTaskStartScheduler();
    while (true);
}
