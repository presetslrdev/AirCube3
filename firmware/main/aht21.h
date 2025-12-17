#ifndef AHT21_H
#define AHT21_H

#include <stdint.h>

void aht21_init(void);
void aht21_read_data(void);
float aht21_get_temperature_c(void);
float aht21_get_humidity(void);
uint8_t aht21_get_status(void);
void aht21_get_ens16x_compensation(uint8_t *t, uint8_t *h);

#endif // AHT21_H
