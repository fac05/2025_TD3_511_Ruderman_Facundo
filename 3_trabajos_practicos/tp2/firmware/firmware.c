#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Declara un handle para la cola donde guardo los valores del ADC
QueueHandle_t queue_sensor;

// Define la rutina de interrupción del ADC
void adc_ISR(void)
{
    // Variable para el cambio de contexto (de tarea) desde la ISR
    static BaseType_t xHPTW = pdFALSE;
    // Variable local estática para almacenar la lectura del ADC
    static uint16_t adc_value;
    // Desactiva la interrupción del ADC
    adc_irq_set_enabled(false);
    // Detiene la conversión del ADC
    adc_run(false);
    // Obtiene el valor convertido desde el FIFO del ADC
    adc_value = adc_fifo_get();
    // Limpia cualquier valor residual en el FIFO del ADC
    adc_fifo_drain();
    // Envía el valor leído a la cola desde la ISR
    xQueueSendFromISR(queue_sensor, &adc_value, &xHPTW);
}

// Tarea que lee el ADC
static void task_ADC(void *pvParam)
{
    while(1){
        // Habilita la interrupción del ADC
        adc_irq_set_enabled(true);
        // Inicia la conversión del ADC
        adc_run(true);
        // Espera 1 segundo antes de la próxima conversión
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Tarea que escribe por consola la temperatura
static void task_print(void *pvParam)
{
    // Declaracion de variables que requiere el sensor
    uint16_t raw;
    float voltaje;
    float temperatura;

    while(1){
        // Espera a recibir un valor de la cola (bloqueante)
        if(xQueueReceive(queue_sensor, &raw, portMAX_DELAY) == pdPASS){
            // Convierte el valor crudo del ADC a voltaje
            voltaje = raw * 3.3 / (1 << 12);
            // Calcula la temperatura con la fórmula del datasheet
            temperatura = 27 - (voltaje - 0.706)/0.001721;
            // Imprime la temperatura calculada
            printf("Temperatura: %.2f C\n\n", temperatura);
        }
    }
}

// Función principal
int main()
{
    stdio_init_all();
    // Inicializcion del ADC
    adc_init();
    // Inicia el sensor de temperatura conectado al ADC
    adc_set_temp_sensor_enabled(true);
    // Selecciono el canal donde está conectado el ADC (el sensor de temp es el 4)
    adc_select_input(4);

    // Configura el FIFO del ADC: habilitado, sin error, genera IRQ en cada muestra
    adc_fifo_setup(true, false, 1, false, false);
    // Habilita la interrupción del ADC
    adc_irq_set_enabled(true);
    // En cuanto haya una interrupcion del ADC, que llame a la función adc_ISR
    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_ISR);
    // Habilita la interrupción del ADC en el NVIC
    irq_set_enabled(ADC_IRQ_FIFO, true);
    
    // Creo la cola donde guardo la temperatura
    queue_sensor = xQueueCreate(1, sizeof(uint16_t));
    
    // Creacion de tareas
    xTaskCreate(task_ADC, "ADC", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(task_print, "ImprimoTemp", 2*configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    
    // Arranco el SO
    vTaskStartScheduler();
    while (true);
}
