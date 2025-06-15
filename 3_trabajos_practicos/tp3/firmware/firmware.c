#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "helper.h"   //Libreria con PWM
#include "lcd.h"      //Libreria de LCD por I2C

#define I2C         i2c0
#define SDA_GPIO    8
#define SCL_GPIO    9
#define ADDR        0x27

TaskHandle_t handle_Task_Frecuencimetro = NULL; //Handler de la tarea frecuencimetro
SemaphoreHandle_t semphrCounting; //Handler del semaforo donde contaré los pulsos

#define PWM_IN  2     //GPIO 2 entrada PWM
#define PWM_OUT 3     //GPIO 3 salida PWM

void ISR_PWM_IN(uint gpio, uint32_t events) { // ISR del pin 2. Parámetros: pin y flag (flanco ascendente)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE; // Variable tipo 'int' para ver si cambiamos de tarea al salir de la ISR

    xSemaphoreGiveFromISR(semphrCounting, &xHigherPriorityTaskWoken); // Le suma uno al semaforo de counting
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Pasa a la tarea mas importante siguiente
    
}

void task_frecuencimetro(void *params) {
    
    TickType_t tiempo_ms = xTaskGetTickCount();
    int contador = 0;
    char str1[20]="La frecuencia es:";
    char str2[20]="";

    gpio_set_irq_enabled_with_callback(PWM_IN, GPIO_IRQ_EDGE_RISE, true, &ISR_PWM_IN); // Habilito la ISR del GPIO, indicando pin y flag (pulso ascendente)

        while(1) {

            if(xTaskGetTickCount() - tiempo_ms > 1000){
                xSemaphoreTake(semphrCounting,0);
                contador = uxSemaphoreGetCount(semphrCounting);
                lcd_clear();
                lcd_set_cursor(0,0);
                lcd_string(str1);
                lcd_set_cursor(1, 0);
                sprintf(str2, "%i Hz", contador);
                lcd_string(str2);
              
                tiempo_ms = xTaskGetTickCount(); // Actualizo el tiempo de referencia para el próximo segundo
                xQueueReset(semphrCounting); // Reinicio el contador del semáforo para contar nuevamente
            }
            vTaskDelay(pdMS_TO_TICKS(20));  // Dejo respirar al micro
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
    pwm_user_init(PWM_OUT, 10000);
    
    //Creacion de Semaforo Counting
    semphrCounting = xSemaphoreCreateCounting( 10000, 0 );  // Le pongo un valor tope por las dudas
    
    // Creacion de tareas
    xTaskCreate(task_frecuencimetro, "Task_Frecuencimetro", 4*configMINIMAL_STACK_SIZE, NULL, 2, NULL);

    // Inicializo el I2C con un clock de 100 KHz (valor típico en una comunicación I2C básica)
    i2c_init(I2C, 10000);
    // Habilito la funcion de I2C en los GPIOs
    gpio_set_function(SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(SCL_GPIO, GPIO_FUNC_I2C);
    // Habilito pull-ups
    gpio_pull_up(SDA_GPIO);
    gpio_pull_up(SCL_GPIO);
    // Inicializo LCD
    lcd_init(I2C, ADDR);
    // Limpio el LCD
    lcd_clear();

    // Arranca el scheduler
    vTaskStartScheduler();
    while(1);
}