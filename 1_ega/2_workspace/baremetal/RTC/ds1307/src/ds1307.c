#include "ds1307.h"
#include "hardware/i2c.h"

#define DS1307_ADDR 0x68
#define AT24C32_ADDR  0x50      // Dirección I2C de la EEPROM
#define I2C_PORT      i2c1      // I2C usado


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

uint8_t eeprom_write(uint8_t *data, uint16_t address, uint8_t bytes)
{
    uint8_t len = bytes + 2;
    // Frame = Address + Data
    uint8_t frame[len];
    static uint8_t status;
    static int resp;
    status = 1;
    
    // Chequeo que no voy a querer escribir mas que 32 bytes (eeprom page)
    if (bytes <= 32){
        // Desempaqueto la direccion de memoria a la que escribir
        frame[0] = (uint8_t)((address >> 8) & 0x00FF);
        frame[1] = (uint8_t)(address & 0x00FF);
        // Cargo los datos en la trama
        for(uint8_t i = 0; i<bytes; i++) frame[i+2] = data[i];
        // frame[2] = data[0];
        // frame[3] = data[1];
        // frame[4] = data[2];
        // frame[5] = data[3];
        // Envio bytes a escribir
        resp = i2c_write_blocking(i2c1, AT24C32_ADDR, frame, len, false);
        // Tiempo de escritura en eeprom PROBAR
        sleep_ms(10);
        if (resp != len) status = 0;
    }
    else status = 0;
    return status;
}

uint8_t eeprom_read(uint8_t *data, uint16_t address, uint8_t bytes)
{
    static uint8_t addr[2];
    static uint8_t status;
    static int resp;
    status = 1;

    // Desempaqueto la direccion de memoria a la que escribir
    addr[0] = (uint8_t)(address >> 8);
    addr[0] = (uint8_t)(address & 0x00FF);
    // Envio direccion de memoria de la eeprom
    resp = i2c_write_blocking(i2c1, AT24C32_ADDR, addr, 2, true);
    if (resp != 2) status = 0;
    // Leo bytes de la eeprom
    resp = i2c_read_blocking(i2c1, AT24C32_ADDR, data, bytes, false);
    if (resp != bytes) status = 0;
    return status;
}