#ifndef DS1307_H
#define DS1307_H

#include "hardware/i2c.h"
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#define DS1307_ADDR 0x68

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day_of_week;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} ds1307_time_t;

void ds1307_init(i2c_inst_t *i2c);
void ds1307_set_time(i2c_inst_t *i2c, const ds1307_time_t *time);
void ds1307_get_time(i2c_inst_t *i2c, ds1307_time_t *time);

bool ds1307_write_byte(i2c_inst_t *i2c, uint8_t address, uint8_t data);
bool ds1307_read_byte(i2c_inst_t *i2c, uint8_t address, uint8_t *data);

// Funciones para escribir y leer en la EEPROM AT24C32
void at24c32_write(uint16_t addr, const uint8_t *data, size_t len);
void at24c32_read(uint16_t addr, uint8_t *data, size_t len);

#endif