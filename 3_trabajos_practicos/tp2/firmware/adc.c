#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "queue.h"

int main() {
    // Inicializacion del ADC  
    adc_init();
    // Inicia el sensor de temperatura conectado al ADC
    adc_set_temp_sensor_enabled(true);
    // Selecciono el canal donde está conectado el ADC (el sensor de temp es el 4)
    adc_select_input(4);

    q_adc  = xQueueCreate
}


QueueHandle_t q_adc;


void task_print(void *params) {
    while(1)
        {

        }
}

void task_adc(void *params) {
    while(1)
    {
        
    }

}