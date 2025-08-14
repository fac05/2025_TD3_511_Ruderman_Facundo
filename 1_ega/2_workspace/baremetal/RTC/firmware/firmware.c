#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "helper.h"   //Libreria con PWM
#include "lcd.h"      //Libreria de LCD por I2C
#include "ds1307.h"

#define SDA_PIN 18
#define SCL_PIN 19
#define I2C_PORT i2c1

void task_rtc(void *params) {
    ds1307_time_t t;
    uint8_t read_value;
    const char *mensaje = "Hola chat, como anda?";
    uint8_t buffer[25] = {0};

    while (1) {
        ds1307_get_time(I2C_PORT, &t);
        printf("%02d:%02d:%02d  %02d/%02d/%02d\n",
            t.hours, t.minutes, t.seconds,
            t.day, t.month, t.year);
        vTaskDelay(pdMS_TO_TICKS(1000));

       // Mensaje a guardar
        eeprom_write((uint8_t *)mensaje, 0x0000, strlen(mensaje));

        vTaskDelay(pdMS_TO_TICKS(1000));

        // Leer de vuelta
        eeprom_read(buffer, 0x0000, strlen(mensaje));

        // Imprimir el contenido leído
        printf("Leído de EEPROM: %s\n", buffer);

        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}

int main() {
    stdio_init_all();
    i2c_init(I2C_PORT, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    ds1307_init(I2C_PORT);

    /*ds1307_time_t new_time = {
        .seconds = 0,
        .minutes = 00,
        .hours = 17,
        .day_of_week = 4,  // Sabado
        .day = 10,
        .month = 8,
        .year = 25         // Año 2024
    };
    ds1307_set_time(I2C_PORT, &new_time); */


    xTaskCreate(task_rtc, "RTC", 1024, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1);
}