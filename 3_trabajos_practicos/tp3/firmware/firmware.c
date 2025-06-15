#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "helper.h"   //Libreria con PWM

TaskHandle_t handle_Task_Frecuencimetro = NULL; //Handler de la tarea frecuencimetro
SemaphoreHandle_t semphrCounting; //Handler del semaforo donde contaré los pulsos

#define PWM_IN  2     //GPIO 2 entrada PWM
#define PWM_OUT 3     //GPIO 3 salida PWM

void task_frecuencimetro(void *params) {
    
    bool estado_ant = false;
    bool estado_act = false;
    TickType_t tiempo_ms = xTaskGetTickCount();
    int contador = 0;
    
    while(1) {
        
        estado_ant = estado_act;
        estado_act = gpio_get(PWM_IN);

        if(estado_act && !estado_ant) // Si detecta flanco ascendente
        {
            xSemaphoreGive(semphrCounting); // Es el equivalente a contador++, pero para FreeRTOS
        }

        if(xTaskGetTickCount() - tiempo_ms > 1000) // Chequeo que haya pasado más de un segundo
        {
            xSemaphoreTake( semphrCounting, 0 ); // Hago un take para ser mas preciso nomas.
            contador = uxSemaphoreGetCount(semphrCounting);
            printf("La frecuencia es %i Hz\n", contador);
            tiempo_ms = xTaskGetTickCount();
            xQueueReset(semphrCounting);
        }

    }
}

/**
 * @brief Programa principal
 */
int main(void) {
    stdio_init_all(); // Inicializo el stdio

    // Inicializacion del pin que lee el PWM
    gpio_init(PWM_IN);
    gpio_set_dir(PWM_IN, false);
    gpio_pull_down(PWM_IN);

    // Generador de PWM
    pwm_user_init(PWM_OUT, 5000);
    
    //Creacion de Semaforo Counting
    semphrCounting = xSemaphoreCreateCounting( 10000, 0 );  // Le pongo un valor tope por las dudas
    
    // Creacion de tareas
    xTaskCreate(task_frecuencimetro, "Task_Frecuencimetro", 4*configMINIMAL_STACK_SIZE, NULL, 2, NULL);

    // Inicio el scheduler
    vTaskStartScheduler();
    while(1);

}