#include "ds1307.h"
#include "hardware/i2c.h"

#define DS1307_ADDR 0x68
#define AT24C32_ADDR  0x50      // Dirección I2C de la EEPROM
#define I2C_PORT      i2c0      // I2C usado


static uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

static uint8_t bcd_to_dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

void ds1307_init(i2c_inst_t *i2c) {
    // No se necesita configuración específica
}

void ds1307_set_time(i2c_inst_t *i2c, const ds1307_time_t *time) {
    uint8_t buffer[8];
    buffer[0] = 0x00;
    buffer[1] = dec_to_bcd(time->seconds);
    buffer[2] = dec_to_bcd(time->minutes);
    buffer[3] = dec_to_bcd(time->hours);
    buffer[4] = dec_to_bcd(time->day_of_week);
    buffer[5] = dec_to_bcd(time->day);
    buffer[6] = dec_to_bcd(time->month);
    buffer[7] = dec_to_bcd(time->year);
    i2c_write_blocking(i2c, DS1307_ADDR, buffer, 8, false);
}

void ds1307_get_time(i2c_inst_t *i2c, ds1307_time_t *time) {
    uint8_t reg = 0x00;
    uint8_t buffer[7];
    i2c_write_blocking(i2c, DS1307_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c, DS1307_ADDR, buffer, 7, false);

    time->seconds      = bcd_to_dec(buffer[0] & 0x7F);
    time->minutes      = bcd_to_dec(buffer[1]);
    time->hours        = bcd_to_dec(buffer[2] & 0x3F);
    time->day_of_week  = bcd_to_dec(buffer[3]);
    time->day          = bcd_to_dec(buffer[4]);
    time->month        = bcd_to_dec(buffer[5]);
    time->year         = bcd_to_dec(buffer[6]);
}

// Escribe un byte en la dirección de RAM del DS1307
bool ds1307_write_byte(i2c_inst_t *i2c, uint8_t address, uint8_t data) {
    if (address < 0x08 || address > 0x3F) return false;

    uint8_t buf[2] = {address, data};
    return (i2c_write_blocking(i2c, DS1307_ADDR, buf, 2, false) == 2);
}

// Lee un byte desde la RAM del DS1307
bool ds1307_read_byte(i2c_inst_t *i2c, uint8_t address, uint8_t *data) {
    if (address < 0x08 || address > 0x3F) return false;

    i2c_write_blocking(i2c, DS1307_ADDR, &address, 1, true);
    return (i2c_read_blocking(i2c, DS1307_ADDR, data, 1, false) == 1);
}

void at24c32_write(uint16_t addr, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint8_t buf[3];
        buf[0] = (addr >> 8) & 0xFF; // Dirección alta
        buf[1] = addr & 0xFF;        // Dirección baja
        buf[2] = data[i];            // Byte a escribir

        i2c_write_blocking(I2C_PORT, AT24C32_ADDR, buf, 3, false);
        addr++;
    }
}

void at24c32_read(uint16_t addr, uint8_t *data, size_t len) {
    uint8_t addr_buf[2] = {
        (addr >> 8) & 0xFF,
        addr & 0xFF
    };
    i2c_write_blocking(I2C_PORT, AT24C32_ADDR, addr_buf, 2, true);
    i2c_read_blocking(I2C_PORT, AT24C32_ADDR, data, len, false);
}