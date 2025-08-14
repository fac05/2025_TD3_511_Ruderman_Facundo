#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "queue.h"

//CONSTANTES DEL TECLADO
#define FILAS 4
#define COLUMNAS 4
#define MAX_INPUT 32
// Eleccion de GPIO para SDA y SCL para el LCD
#define SDA_GPIO    14
#define SCL_GPIO    15
//Eleccion de pines
#define IN_PIN_TACOMETRO 27
const uint FILA_PINS[FILAS] = {6, 7, 8, 9};
const uint COLUMNA_PINS[COLUMNAS] = {10, 11, 12, 13};

//Defino mi cola
QueueHandle_t q_matrix ;
QueueHandle_t q_tacometro ;

//Defino las teclas de mi teclado
const char teclas[FILAS][COLUMNAS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
#define TRUE 1
#define FALSE 0

void configurar_gpio() {
    for (int i = 0; i < FILAS; i++) {
        gpio_init(FILA_PINS[i]);
        gpio_set_dir(FILA_PINS[i], GPIO_OUT);
        gpio_put(FILA_PINS[i], 1);
    }
    for (int i = 0; i < COLUMNAS; i++) {
        gpio_init(COLUMNA_PINS[i]);
        gpio_set_dir(COLUMNA_PINS[i], GPIO_IN);
        gpio_pull_up(COLUMNA_PINS[i]);
    }
    printf("Listo para leer el teclado 4x4...\n");
}

char escanear_teclado() {
    for (int f = 0; f < FILAS; f++) {
        for (int i = 0; i < FILAS; i++)
            gpio_put(FILA_PINS[i], i == f ? 0 : 1);

        vTaskDelay(pdMS_TO_TICKS(5));  // estabilización rápida

        for (int c = 0; c < COLUMNAS; c++) {
            if (gpio_get(COLUMNA_PINS[c]) == 0) {
                return teclas[f][c];
            }
        }
    }
    return 0;
}

void task_matrix(void *params) {
    char buffer[MAX_INPUT + 1] = {0};
    int index = 0;
    char tecla_anterior = 0;
    

    while (true) {
        char tecla_actual = escanear_teclado();

        if (tecla_actual != 0 && tecla_actual != tecla_anterior) {
            //printf("Tecla presionada: %c\n", tecla_actual);

            if (index < MAX_INPUT && tecla_actual != '#') {
                buffer[index++] = tecla_actual;
            }

            if (tecla_actual == '#') {
                buffer[index] = '\0';
                printf("Ingresado: %s\n", buffer);
                index = 0;
                xQueueSend(q_matrix , buffer , portMAX_DELAY );
            }
        }

        tecla_anterior = tecla_actual;

        vTaskDelay(pdMS_TO_TICKS(50));  // evita múltiples lecturas rápidas
    }
}

void task_lcd(void *params) {
    char str_matrix[17] = {0};
    char str_taco[17] = {0};
    char matrix_input[MAX_INPUT + 1] = {0};
    float taco_input = 0;
    bool cambio = FALSE;

    while (true) {
        

        if (xQueueReceive(q_matrix, &matrix_input, 0) == pdPASS) {
            snprintf(str_matrix, sizeof(str_matrix), "Ingresado: %s", matrix_input);
            cambio = TRUE;
        }

        if (xQueueReceive(q_tacometro, &taco_input, 0) == pdPASS) {
            snprintf(str_taco, sizeof(str_taco), "Velocidad: %.2f", taco_input);
            cambio = TRUE;
        }

        if (cambio==TRUE) {
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_string(str_matrix);
            lcd_set_cursor(1, 0);
            lcd_string(str_taco);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}
void Tacometro(void *params) {
    int counter = 0;
    float vueltas = 0;
    int estado_anterior = gpio_get(IN_PIN_TACOMETRO);
    char str[16];

    TickType_t start_tick = xTaskGetTickCount();  // tiempo de referencia
    const TickType_t periodo = pdMS_TO_TICKS(1000);  // 1 segundo

    while (true) {
        int estado_actual = gpio_get(IN_PIN_TACOMETRO);

        // Detectar cambio de estado (flanco de subida)
        if (estado_actual != estado_anterior) {
            estado_anterior = estado_actual;

            if (estado_actual == 1) {
                counter++;
            }
        }

        // ¿Pasó 1 segundo?
        if ((xTaskGetTickCount() - start_tick) >= periodo) {
            vueltas = counter / 21.0f;
            printf("Activaciones por segundo: %d\n", counter);
            printf("Vueltas por segundo: %.2f\n", vueltas);
            xQueueSend(q_tacometro , &vueltas , portMAX_DELAY );
            counter = 0;
            vueltas = 0;
            start_tick = xTaskGetTickCount();  // reinicio del conteo

            
            
        }

        vTaskDelay(pdMS_TO_TICKS(1));  // respiro para evitar uso 100% CPU
    }
}
int main() {
    stdio_init_all();
    //CONFIGURACION DEL TECLADO
    configurar_gpio();
    //CONFIGURACION DEL TACOMETRO
    gpio_init(IN_PIN_TACOMETRO);
    gpio_set_dir(IN_PIN_TACOMETRO, GPIO_IN);
    gpio_pull_down(IN_PIN_TACOMETRO);
    //CONFIUGRACION DEL DISPLAY
    i2c_init(i2c1, 100000);
    //Seteo la funcion
    gpio_set_function(SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(SCL_GPIO, GPIO_FUNC_I2C);
    // Habilito pull-ups
    gpio_pull_up(SDA_GPIO);
    gpio_pull_up(SCL_GPIO);
    //Inicializo el display
    lcd_init(i2c1, 0x27 );
    lcd_clear();
    
    //Inicializo mis colas
    q_matrix = xQueueCreate(1 , sizeof(char[MAX_INPUT + 1]));
    q_tacometro = xQueueCreate(1 , sizeof(float) );

    //Inicializo mis tareas
    xTaskCreate(task_matrix, "Matrix", 4 * configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(task_lcd, "LCD",  configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(Tacometro, "Tacometro", 2 * configMINIMAL_STACK_SIZE , NULL, 2, NULL);

    //Arranca el scheduler
    vTaskStartScheduler();

    while (1) {}
}
