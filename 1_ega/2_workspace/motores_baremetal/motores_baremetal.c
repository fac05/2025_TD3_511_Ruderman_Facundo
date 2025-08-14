#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
// Pines para control de dirección y PWM
#define MOTOR_M2_IN1 18
#define MOTOR_M2_IN2 19
#define MOTOR_M1_IN1 20
#define MOTOR_M1_IN2 21
#define MOTOR_M2_PWM 22  // GPIO con soporte PWM
#define MOTOR_M1_PWM 26  // GPIO con soporte PWM
// Configuración de la frecuencia PWM (Hz)
#define PWM_FREQ 1000

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

// Cambiar velocidad (0 a 255)
void motor_set_speed(uint8_t speed) {
    uint slice1 = pwm_gpio_to_slice_num(MOTOR_M1_PWM);
    uint slice2 = pwm_gpio_to_slice_num(MOTOR_M2_PWM);

    pwm_set_chan_level(slice1, pwm_gpio_to_channel(MOTOR_M1_PWM), speed);
    pwm_set_chan_level(slice2, pwm_gpio_to_channel(MOTOR_M2_PWM), speed);
}


int main()
{
    stdio_init_all();

    motor_init();
    motor_set_direction(1,true);  // Sentido "hacia adelante"
    motor_set_direction(2,true);
    uint16_t speed = 255;
    unsigned int sentido = 0;

    while (1) {
        motor_set_speed(speed);
        /*
        if (xQueueReceive(q_vel_deseada, &speed, portMAX_DELAY)) {

            printf("Nueva velocidad recibida: %u\n", speed);

            motor_set_speed(speed);
        }*/
            
           if (sentido == 0){
            speed -= 10;
           } else if (sentido == 1){
            speed += 10; 
           }
           if (speed <= 5){
            sentido = 1; 
           }else if (speed >= 255){
            sentido = 0;
           }
           printf("Nueva velocidad recibida: %u\n", speed);
           sleep_ms(500);  // Reemplazo de vTaskDelay
        
    }
}
