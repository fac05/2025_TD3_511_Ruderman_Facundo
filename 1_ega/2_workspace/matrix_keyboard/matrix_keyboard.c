#include <stdio.h>
#include "pico/stdlib.h"

#define FILAS 4
#define COLUMNAS 4
#define MAX_INPUT 32

// Pines conectados a filas y columnas
const uint FILA_PINS[FILAS] = {6, 7, 8, 9};
const uint COLUMNA_PINS[COLUMNAS] = {10, 11, 12, 13};


// Mapa del teclado
const char teclas[FILAS][COLUMNAS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

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
}

char escanear_teclado() {
    for (int f = 0; f < FILAS; f++) {
        // Activar fila f
        for (int i = 0; i < FILAS; i++)
            gpio_put(FILA_PINS[i], i == f ? 0 : 1);

        sleep_us(10);  // pequeña espera para estabilizar

        for (int c = 0; c < COLUMNAS; c++) {
            if (gpio_get(COLUMNA_PINS[c])==0) {
                sleep_ms(150);  // debounce simple
                //printf("Tecla presionada: %c\n", teclas[f][c]); 
                return teclas[f][c];
            }
        }
    }
    return 0;
}

int main() {
    stdio_init_all();
    configurar_gpio();
    printf("Listo para leer el teclado 4x4...\n");
    char buffer[MAX_INPUT + 1] = {0};
    int index = 0;
    while (1) {
        char tecla = escanear_teclado();  // retorna tecla presionada o 0
    
        if (tecla) {
            printf ("Tecla presionada: %c\n", tecla);
            if (index< MAX_INPUT && tecla != '#' ){
                buffer[index++]=tecla;
            }
            if (tecla == '#') {
                printf("Ingresado: %s \n", buffer);
                index = 0;
                buffer[0] = '\0';
                
                //fflush(stdout);
            }
            // Usamos la tecla
            //printf("Tecla presionada: %c\n", tecla_actual);
    
            // "Olvidamos" el valor
            //tecla_actual = 0;
        }
    
        sleep_ms(50);
    }
    
}
