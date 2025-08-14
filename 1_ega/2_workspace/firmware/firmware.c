#include <stdio.h>
#include "pico/stdlib.h"
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "hardware/pwm.h"
#include "string.h"

// MACROS
// Pines para control de dirección y PWM
#define MOTOR_M2_IN1 18
#define MOTOR_M2_IN2 19
#define MOTOR_M1_IN1 20
#define MOTOR_M1_IN2 21
#define MOTOR_M2_PWM 22  // GPIO con soporte PWM
#define MOTOR_M1_PWM 26  // GPIO con soporte PWM
// Configuración de la frecuencia PWM (Hz)
#define PWM_FREQ 15000
// Cambiar velocidad (0 a 255)
#define MOTOR_LEFT      1
#define MOTOR_RIGHT     2
//Contastante de conversion de velocidad a PWN
#define PENDIENTE   0.00326
#define ORDENADA    -0.5684
//Modos 
//SEGUN EL MODO, PONEMOS LA VELOCIDAD
#define VEL_01  0.175
#define VEL_02  0.19
#define VEL_03  0.205
#define CURVA_01 0.3
#define CURVA_02 0.4
#define CURVA_03 0.5
//Cosntantes del auto
#define RADIUS 0.134 //radio entre el centro de las ruedas en m
//Configuración del teclado matricial
#define FILAS 4
#define COLUMNAS 4
#define MAX_INPUT 4


typedef struct {
    float v_right;
    float v_left;
} VelData_t;

const uint FILA_PINS[FILAS] = {6, 7, 8, 9};
const uint COLUMNA_PINS[COLUMNAS] = {10, 11, 12, 13};
const char teclas[FILAS][COLUMNAS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

QueueHandle_t q_codigo;
QueueHandle_t q_vel_deseada;

void configurar_gpio() {
    //INICIALIZACION DEL TACOMETRO
    //gpio_init(IN_PIN_TACOMETRO);
    //gpio_set_dir(IN_PIN_TACOMETRO, GPIO_IN);
    //gpio_pull_down(IN_PIN_TACOMETRO);

    //INICIALIZACION DEL TECLADO MATRICIAL
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

    //INICIALIZACION DEL MOTOR
    //motor_init();
    //motor_set_direction(true);  
}
char escanear_teclado() {
    for (int f = 0; f < FILAS; f++) {
        for (int i = 0; i < FILAS; i++)
            gpio_put(FILA_PINS[i], i == f ? 0 : 1);

        vTaskDelay(pdMS_TO_TICKS(30));  // estabilización rápida

        for (int c = 0; c < COLUMNAS; c++) {
            if (gpio_get(COLUMNA_PINS[c]) == 0) {
                return teclas[f][c];
            }
        }
    }
    return 0;
}
void motor_init() {
    float clock = 125000000 ;
    float divider = clock / (PWM_FREQ * (255+1));
    // Dirección
    gpio_init(MOTOR_M1_IN1);
    gpio_set_dir(MOTOR_M1_IN1, GPIO_OUT);
    gpio_put(MOTOR_M1_IN1, 0);

    gpio_init(MOTOR_M1_IN2);
    gpio_set_dir(MOTOR_M1_IN2, GPIO_OUT);
    gpio_put(MOTOR_M1_IN2, 0);

    gpio_init(MOTOR_M2_IN1);
    gpio_set_dir(MOTOR_M2_IN1, GPIO_OUT);
    gpio_put(MOTOR_M2_IN1, 0);

    gpio_init(MOTOR_M2_IN2);
    gpio_set_dir(MOTOR_M2_IN2, GPIO_OUT);
    gpio_put(MOTOR_M2_IN2, 0);


    // PWM Motor 1
    gpio_set_function(MOTOR_M1_PWM, GPIO_FUNC_PWM);
    uint slice1 = pwm_gpio_to_slice_num(MOTOR_M1_PWM);
    pwm_set_clkdiv(slice1, divider);
    pwm_set_wrap(slice1, 255);
    pwm_set_chan_level(slice1, pwm_gpio_to_channel(MOTOR_M1_PWM), 0);
    pwm_set_enabled(slice1, true);

    // PWM Motor 2
    gpio_set_function(MOTOR_M2_PWM, GPIO_FUNC_PWM);
    uint slice2 = pwm_gpio_to_slice_num(MOTOR_M2_PWM);
    pwm_set_clkdiv(slice2, divider);
    pwm_set_wrap(slice2, 255);
    pwm_set_chan_level(slice2, pwm_gpio_to_channel(MOTOR_M2_PWM), 0);
    pwm_set_enabled(slice2, true);
}

// Cambiar dirección del motor
void motor_set_direction(uint8_t motor, bool forward) {
    if (motor == 1) {
        gpio_put(MOTOR_M1_IN1, forward ? 1 : 0);
        gpio_put(MOTOR_M1_IN2, forward ? 0 : 1);
    } else if (motor == 2) {
        gpio_put(MOTOR_M2_IN1, forward ? 1 : 0);
        gpio_put(MOTOR_M2_IN2, forward ? 0 : 1);
    }
}



void motor_set_speed(uint8_t motor, uint8_t speed) {
    if (motor == MOTOR_LEFT) {
        uint slice1 = pwm_gpio_to_slice_num(MOTOR_M1_PWM);
        pwm_set_chan_level(slice1, pwm_gpio_to_channel(MOTOR_M1_PWM), speed);
    }
    else if (motor == MOTOR_RIGHT) {
        uint slice2 = pwm_gpio_to_slice_num(MOTOR_M2_PWM);
        pwm_set_chan_level(slice2, pwm_gpio_to_channel(MOTOR_M2_PWM), speed);
    }
}



void task_motor(void *params) {
    motor_init();
    motor_set_direction(1,false);  // Sentido "hacia adelante"
    motor_set_direction(2,false);
    uint16_t linear_speed = 0;
    uint16_t radius = 0;
    float omega = 0;
    char buffer[MAX_INPUT + 1] = {0};
    VelData_t diff_vel ;
    //unsigned int sentido = 0;

    while (1) {
        //motor_set_speed(linear_speed);
        
        //printf("Estoy en la task del motor");

        if (xQueueReceive(q_codigo, &buffer, portMAX_DELAY)) {
            printf("Recibi info del teclado \n");
            
            if      (buffer[0]=='1'){linear_speed=VEL_01;}
            else if (buffer[0]=='2'){linear_speed=VEL_02;}
            else if (buffer[0]=='3'){linear_speed=VEL_03;}
            

            if      (buffer[1]=='A'){radius=CURVA_01;}
            else if (buffer[1]=='B'){radius=CURVA_02;}
            else if (buffer[1]=='C'){radius=CURVA_03;}
            //else continue;

            omega = linear_speed / radius ;
            diff_vel.v_left     = linear_speed - RADIUS *   omega / 2;
            diff_vel.v_right    = linear_speed + RADIUS *   omega / 2;
            
            //printf("linear_speed = %u, radius = %u, omega = %.2f\n", linear_speed, radius, omega);
            //printf("diff_vel.v_left = %.2f, diff_vel.v_right = %.2f\n", diff_vel.v_left, diff_vel.v_right);
            
            xQueueOverwrite(q_vel_deseada, &diff_vel);
            //motor_set_speed(MOTOR_LEFT , diff_vel.v_left );
            //motor_set_speed(MOTOR_RIGHT , diff_vel.v_right );
            //motor_set_speed(speed);
            
            vTaskDelay(pdMS_TO_TICKS(2500));
            
        }
        motor_set_speed(MOTOR_LEFT , 0 );
        motor_set_speed(MOTOR_RIGHT , 0);
           //speed = 0;
        vTaskDelay(pdMS_TO_TICKS(100));
        
    }
}

void task_PID_right(void *pvParameters) {
    VelData_t vel_setpoint;
    float pulsos = 0 ;

    while (1) {
        // Espera hasta recibir datos de la cola
        if (xQueueReceive(q_vel_deseada, &vel_setpoint, portMAX_DELAY) == pdPASS) {
            // Setea velocidad del motor indicado
            pulsos = (vel_setpoint.v_right - ORDENADA)/PENDIENTE ;
            motor_set_speed(MOTOR_RIGHT, pulsos);

        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_PID_left(void *pvParameters) {
    VelData_t vel_setpoint;
    float pulsos = 0 ;

    while (1) {/*
        // Espera hasta recibir datos de la cola
        if (xQueuePeek(q_vel_deseada, &vel_setpoint, portMAX_DELAY) == pdPASS) {
            // Setea velocidad del motor indicado
            pulsos = (vel_setpoint.v_right - ORDENADA)/PENDIENTE ;
            motor_set_speed(MOTOR_LEFT, pulsos);
        }*/
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_matrix(void *params) {
    char buffer[MAX_INPUT + 1] = {0};
    int index = 2;
    char tecla_anterior = 0;
    char tecla_actual = 0;
    int cod_num = 0;
    int cod_letter = 0 ;
    

    while (true) {
        tecla_actual = escanear_teclado();
        if (tecla_actual != 0 && tecla_actual != tecla_anterior) {
            printf("Tecla presionada: %c\n", tecla_actual);

            if (index < MAX_INPUT && tecla_actual != '#') {
                //
                if (tecla_actual == '1' || tecla_actual == '2' || tecla_actual == '3') {
                    buffer[0] = tecla_actual;
                    cod_num = 1;
                }else if (tecla_actual == 'A' || tecla_actual == 'B' || tecla_actual == 'C') {
                    buffer[1] = tecla_actual;
                    cod_letter = 1 ;
                }else {buffer[index++] = tecla_actual;}
            }
            

            if (tecla_actual == '#') {
                buffer[2] = '\0';
                printf("Ingresado: %s\n", buffer);
                index = 2;
                xQueueSend(q_codigo , buffer , 0 );
                printf("Ya paso por la cola \n");
            }
        }
        tecla_anterior = tecla_actual;

        vTaskDelay(pdMS_TO_TICKS(100)); 
    }

        
        /*
        if (tecla_actual != 0 && tecla_actual != tecla_anterior) {
            printf("Tecla presionada: %c\n", tecla_actual);
            
            

            if (tecla_actual == '*') {
                index = 0;
                memset(buffer, 0, sizeof(buffer));
                //printf("Buffer borrado");
            }
            printf("Stack libre: %u\n", uxTaskGetStackHighWaterMark(NULL));

            if (tecla_actual == '#') {
                // Solo enviar si buffer[0] y buffer[1] no están vacíos
                
                //printf("Ingresado antes de entrar al IF: %s\n", buffer);
                if (cod_num == 1 && cod_letter==1) {
                    buffer[2] = '\0'; // Cierro string
                    printf("Ingresado: %s\n", buffer);
                    
                    //xQueueSend(q_codigo, buffer, portMAX_DELAY);
                    if (xQueueSend(q_codigo, buffer, 0) != pdPASS) {
                        printf("Cola llena, no pude enviar.\n");
                    }
                    // Limpiar después de enviar
                    index = 0;
                    cod_num = 0;
                    cod_letter = 0 ;
                    
                } 
                    else {
                    printf("Error: faltan datos antes de enviar.\n");
                    //memset(buffer, 0, sizeof(buffer));
                }
                
            }*/
        

    // evita múltiples lecturas rápidas
}


int main()
{
    stdio_init_all();
    configurar_gpio();
    //INICIALIZACION DE LAS COLAS 
    q_codigo        = xQueueCreate(100 , sizeof(char[MAX_INPUT + 1])  );
    q_vel_deseada   = xQueueCreate(10 , sizeof(VelData_t)            );

    //INICIALIZACION DE LAS TAREAS 
    xTaskCreate(task_matrix     , "Matrix"      , 5 * configMINIMAL_STACK_SIZE  , NULL, 4, NULL);
    xTaskCreate(task_motor      , "Motor"       , configMINIMAL_STACK_SIZE *2   , NULL, 3, NULL);
    xTaskCreate(task_PID_right  , "Right PID"   , configMINIMAL_STACK_SIZE *2   , NULL, 2, NULL);
    xTaskCreate(task_PID_left   , "Left PID"    , configMINIMAL_STACK_SIZE *2   , NULL, 2, NULL);

    vTaskStartScheduler();
    while (true) {
        
    }
}
